// -*- lsst-c++ -*-

/*
 * LSST Data Management System
 * Copyright 2008, 2009, 2010 LSST Corporation.
 *
 * This product includes software developed by the
 * LSST Project (http://www.lsst.org/).
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the LSST License Statement and
 * the GNU General Public License along with this program.  If not,
 * see <http://www.lsstcorp.org/LegalNotices/>.
 */

/**
* @file
*
* @brief Times the speedup of GPU accelerated convolution
*
* @author Kresimir Cosic
*
* @ingroup afw
*/


#include <typeinfo>
#include <cstdio>

#include <iostream>
#include <sstream>
#include <string>
#include <math.h>

#include "lsst/daf/base.h"
#include "lsst/pex/exceptions.h"
#include "lsst/pex/logging/Trace.h"
#include "lsst/afw/image.h"
#include "lsst/afw/geom.h"
#include "lsst/afw/math.h"
#include "lsst/afw/gpu/IsGpuBuild.h"
#include "lsst/afw/gpu/DevicePreference.h"
//Just for PrintCudaDeviceInfo
#include "lsst/afw/gpu/detail/CudaQueryDevice.h"

const int defaultInterpLen = 20;

typedef int TestResult;

using namespace std;
using lsst::pex::logging::Trace;
namespace pexEx = lsst::pex::exceptions;
namespace afwImage = lsst::afw::image;
namespace afwMath  = lsst::afw::math;
namespace afwGeom  = lsst::afw::geom;

//Calculates relative RMSD (coefficient of variation of the root-mean-square deviation)
template <typename T1, typename T2>
double CvRmsd(const afwImage::Image<T1>& imgA, const afwImage::Image<T2>& imgB)
{
    const int dimX = imgA.getWidth();
    const int dimY = imgA.getHeight();

    if (dimX != imgB.getWidth() || dimY != imgB.getHeight()) return NAN;

    double sqSum = 0;
    double avgSum = 0;
    int cnt = 0;

    for (int x = 0; x < dimX; x++) {
        for (int y = 0; y < dimY; y++) {
            const double valA = imgA(x, y);
            const double valB = imgB(x, y);
            if (isnan(valA) || isnan(valB)) continue;
            if (isinf(valA) || isinf(valB)) continue;

            cnt++;
            avgSum += (valA + valB) / 2;
            const double diff = valA - valB;
            sqSum += diff * diff;
        }
    }
    double rmsd = sqrt(sqSum / cnt);
    double avg = avgSum / cnt;
    return rmsd / avg;
}


//Returns number of different values
template <typename T>
double DiffCnt(const afwImage::Mask<T>& imgA, const afwImage::Mask<T>& imgB)
{
    typedef long long unsigned int Bitint;

    const int dimX = imgA.getWidth();
    const int dimY = imgA.getHeight();

    if (dimX != imgB.getWidth() || dimY != imgB.getHeight()) return NAN;

    int cnt = 0;

    for (int x = 0; x < dimX; x++) {
        for (int y = 0; y < dimY; y++) {
            const T valA = imgA(x, y);
            const T valB = imgB(x, y);

            if (valA != valB) cnt++;
        }
    }
    return cnt;
}


void PrintSeparator()
{
    for (int i = 0; i < 79; i++)
        cout << "=";
    cout << endl;
}

string GetInputFileName(int argc, char **argv)
{
    string imgBaseFileName;
    if (argc < 2) {
        string afwdata = getenv("AFWDATA_DIR");
        if (afwdata.empty()) {
            std::cerr << "Usage: convolveGPU fitsFile" << endl;
            std::cerr << "fitsFile excludes the \"_img.fits\" suffix" << endl;
            std::cerr << "I can take a default file from AFWDATA_DIR, but it's not defined." << endl;
            std::cerr << "Is afwdata set up?\n" << endl;
            exit(EXIT_FAILURE);
        }
        else {
            imgBaseFileName = afwdata + "/data/med";
            //imgBaseFileName = afwdata + "/data/medsub";
            //imgBaseFileName = afwdata + "/data/871034p_1_MI";
            cout << "Using image: " << imgBaseFileName << endl;
        }
    }
    else {
        imgBaseFileName = string(argv[1]);
    }
    return imgBaseFileName;
}

string Sel(bool b, const char* onTrue, const char* onFalse)
{
    return b ? string(onTrue) : string(onFalse);
}

string DecimalPlaces(int places, double val)
{
    stringstream ss;
    ss << fixed << showpoint << setprecision(places) << val;
    return ss.str();
}

bool IsErrorAcceptable(double val, double limit)
{
    if (isnan(val)) return false;
    if (isinf(val)) return false;
    return val < limit;
}

template<typename T>
typename T::SinglePixel const GetEdgePixel(T& x)
{ 
    return afwMath::edgePixel< T >( typename afwImage::detail::image_traits< T >::image_category() );
}

bool TestWarpGpu(
    const afwImage::MaskedImage<double>  inImgDbl,
    const afwImage::MaskedImage<float>   inImgFlt,
    afwImage::Wcs::Ptr wcs1,
    afwImage::Wcs::Ptr wcs2,
    const int kernelOrder,
    const int interpLen,
    string           wcsStr
)
{
    const afwImage::MaskedImage<double>  inMIDbl = inImgDbl;
    const afwImage::MaskedImage<float>   inMIFlt = inImgFlt;
    const int sizeX = inMIDbl.getWidth();
    const int sizeY = inMIDbl.getHeight();

    const afwImage::Image<double> inPIDbl = *inMIDbl.getImage();
    const afwImage::Image<float>  inPIFlt = *inMIFlt.getImage();

    cout << "Image size: " << sizeX << " x " << sizeY << "   ";
    cout << "Kernel order: " << kernelOrder << "   ";
    cout << "Interpolation length: " << interpLen << "   ";
    cout << endl;

    lsst::afw::gpu::DevicePreference useGpu = lsst::afw::gpu::USE_GPU;
    lsst::afw::gpu::DevicePreference useCpu = lsst::afw::gpu::USE_CPU;

    afwMath::LanczosWarpingKernel lanKernel(kernelOrder);

    afwImage::MaskedImage<double> resMIDbl(inMIDbl.getDimensions());
    afwImage::MaskedImage<float>  resMIFlt(inMIDbl.getDimensions());
    afwImage::MaskedImage<double> resMIDblGpu(inMIDbl.getDimensions());
    afwImage::MaskedImage<float>  resMIFltGpu(inMIDbl.getDimensions());
    afwImage::Image<double>       resPIDbl(inMIDbl.getDimensions());
    afwImage::Image<float>        resPIFlt(inMIDbl.getDimensions());
    afwImage::Image<double>       resPIDblGpu(inMIDbl.getDimensions());
    afwImage::Image<float>        resPIFltGpu(inMIDbl.getDimensions());

    for (int i = 0; i < int(wcsStr.size()); i++)  cout << " ";
    cout << "         Planes          Image      Variance      Mask      Ret" << endl;
    for (int i = 0; i < 79; i++)  cout << "-";
    cout << endl;

    bool isSuccess = true;

    // warp
    int retPIDblCpu = afwMath::warpImage(resPIDbl   , *wcs1, inPIDbl, *wcs2, lanKernel, interpLen, GetEdgePixel(resPIDbl), useCpu);
    int retPIDblGpu = afwMath::warpImage(resPIDblGpu, *wcs1, inPIDbl, *wcs2, lanKernel, interpLen, GetEdgePixel(resPIDbl), useGpu);
    int retPIFltCpu = afwMath::warpImage(resPIFlt   , *wcs1, inPIFlt, *wcs2, lanKernel, interpLen, GetEdgePixel(resPIFlt), useCpu);
    int retPIFltGpu = afwMath::warpImage(resPIFltGpu, *wcs1, inPIFlt, *wcs2, lanKernel, interpLen, GetEdgePixel(resPIFlt), useGpu);

    const double errDbl = 5e-13;
    const double errFlt = 5e-7;
    const int maxRetDiff = sqrt(sqrt(sizeX*sizeY))/3;
    const int maxMskDiff = sqrt(sqrt(sizeX*sizeY))/3;

    double diffPIDbl = CvRmsd(resPIDbl, resPIDblGpu);
    double diffPIFlt = CvRmsd(resPIFlt, resPIFltGpu);
    int    dretPIDbl = retPIDblCpu-retPIDblGpu;
    int    dretPIFlt = retPIFltCpu-retPIFltGpu;

    if ( !IsErrorAcceptable(diffPIDbl, errDbl) ) isSuccess = false;
    if ( !IsErrorAcceptable(diffPIFlt, errFlt) ) isSuccess = false;
    if ( abs(dretPIDbl) > maxRetDiff) isSuccess = false;
    if ( abs(dretPIFlt) > maxRetDiff) isSuccess = false;

    cout << "P. Image Dbl " << wcsStr << " Dev: "
         << setw(11)     << diffPIDbl << Sel(diffPIDbl > errDbl          , "*  ", "   ")
         << setw(14+9+6) << dretPIDbl << Sel(abs(dretPIDbl) > maxRetDiff , "*", " ") << endl;
    cout << "P. Image Flt " << wcsStr << " Dev: "
         << setw(11)     << diffPIFlt << Sel(diffPIFlt > errFlt          , "*  ", "   ")
         << setw(14+9+6) << dretPIFlt << Sel(abs(dretPIFlt) > maxRetDiff , "*", " ") << endl;

    int retMIDblCpu = afwMath::warpImage(resMIDbl   , *wcs1, inMIDbl, *wcs2, lanKernel, interpLen, GetEdgePixel(resMIDbl), useCpu);
    int retMIDblGpu = afwMath::warpImage(resMIDblGpu, *wcs1, inMIDbl, *wcs2, lanKernel, interpLen, GetEdgePixel(resMIDbl), useGpu);
    int retMIFltCpu = afwMath::warpImage(resMIFlt   , *wcs1, inMIFlt, *wcs2, lanKernel, interpLen, GetEdgePixel(resMIFlt), useCpu);
    int retMIFltGpu = afwMath::warpImage(resMIFltGpu, *wcs1, inMIFlt, *wcs2, lanKernel, interpLen, GetEdgePixel(resMIFlt), useGpu);

    double diffMIImgDbl = CvRmsd(*resMIDbl.getImage()   , *resMIDblGpu.getImage());
    double diffMIVarDbl = CvRmsd(*resMIDbl.getVariance(), *resMIDblGpu.getVariance());
    double diffMIMskDbl = DiffCnt(*resMIDbl.getMask()   , *resMIDblGpu.getMask());
    int    dretMIDbl = retMIDblCpu-retMIDblGpu;

    double diffMIImgFlt = CvRmsd(*resMIFlt.getImage()   , *resMIFltGpu.getImage());
    double diffMIVarFlt = CvRmsd(*resMIFlt.getVariance(), *resMIFltGpu.getVariance());
    double diffMIMskFlt = DiffCnt(*resMIFlt.getMask()   , *resMIFltGpu.getMask());
    int    dretMIFlt = retMIFltCpu-retMIFltGpu;

    if ( !IsErrorAcceptable(diffMIImgDbl, errDbl) ) isSuccess = false;
    if ( !IsErrorAcceptable(diffMIVarDbl, errFlt) ) isSuccess = false;
    if ( diffMIMskDbl > maxMskDiff) isSuccess = false;
    if ( abs(dretMIDbl) > maxRetDiff) isSuccess = false;
    if ( !IsErrorAcceptable(diffMIImgFlt, errFlt) ) isSuccess = false;
    if ( !IsErrorAcceptable(diffMIVarFlt, errFlt) ) isSuccess = false;
    if ( diffMIMskFlt > maxMskDiff) isSuccess = false;
    if ( abs(dretMIFlt) > maxRetDiff) isSuccess = false;

    cout << "M. Image Dbl " << wcsStr << " Dev: "
         << setw(11) << diffMIImgDbl << Sel(diffMIImgDbl   > errDbl     , "*  ", "   ")
         << setw(11) << diffMIVarDbl << Sel(diffMIVarDbl   > errFlt     , "*  ", "   ")
         << setw( 6) << diffMIMskDbl << Sel(diffMIMskDbl   > maxMskDiff , "*  ", "   ")
         << setw( 6) << dretMIDbl    << Sel(abs(dretMIDbl) > maxRetDiff , "*", " ")
         << endl;
    cout << "M. Image Flt " << wcsStr << " Dev: "
         << setw(11) << diffMIImgFlt << Sel(diffMIImgFlt   > errFlt     , "*  ", "   ")
         << setw(11) << diffMIVarFlt << Sel(diffMIVarFlt   > errFlt     , "*  ", "   ")
         << setw( 6) << diffMIMskFlt << Sel(diffMIMskFlt   > maxMskDiff , "*  ", "   ")
         << setw( 6) << dretMIFlt    << Sel(abs(dretMIFlt) > maxRetDiff , "*", " ")
         << endl;

    if (!isSuccess) {
        cout << "ERROR: Unacceptaby large deviation found!" << endl;
        cout << "The failed tests are marked with *" << endl;
    }

    return isSuccess;
}

afwImage::Wcs::Ptr GetLinWcs(double x1, double y1, double x2, double y2,
                          afwGeom::Point2D o1=afwGeom::Point2D(0,0),
                          afwGeom::Point2D o2=afwGeom::Point2D(0,0)
                          )
{
    Eigen::Matrix2d  m1(2,2);
    m1 << x1 ,y1, x2, y2;

    return afwImage::Wcs::Ptr(new afwImage::Wcs(o1, o2, m1));
}

bool GpuTestAccuracy(
    const afwImage::MaskedImage<double>&  inImgDblFull,
    const afwImage::MaskedImage<float>&   inImgFltFull
)
{
    lsst::pex::logging::Trace::setDestination(std::cout);
    lsst::pex::logging::Trace::setVerbosity("lsst.afw.kernel", 5);

    afwGeom::Box2I inputBBox(afwGeom::Point2I(11, 19), afwGeom::Extent2I(40, 61));

    const afwImage::MaskedImage<double>  inMIDbl(inImgDblFull, inputBBox,afwImage::LOCAL, true);
    const afwImage::MaskedImage<float>   inMIFlt(inImgFltFull, inputBBox,afwImage::LOCAL, true);

    bool isSuccess = true;

    for (int order=1; order < 7; order++){
        for (int interpLen=1; interpLen < 12; interpLen++){

            PrintSeparator();
            afwImage::Wcs::Ptr wcs1_1=GetLinWcs(1.3  , -0.2,  -0.4,  0.8  );
            afwImage::Wcs::Ptr wcs1_2=GetLinWcs(0.88 ,0.2, -0.4, 1.12);

            const bool isSuccessLin1 = TestWarpGpu(inMIDbl, inMIFlt, wcs1_1, wcs1_2,
                                                  order, interpLen, "[Linear WCS 1]" );

            PrintSeparator();
            afwImage::Wcs::Ptr wcs2_1=GetLinWcs(0.1, 1.1, -1.1, 0.3,
                                              afwGeom::Point2D(3,5) , afwGeom::Point2D(2,11) );
            afwImage::Wcs::Ptr wcs2_2=GetLinWcs(0.88 ,-0.2, 0.4, -1.12,
                                              afwGeom::Point2D(2,1), afwGeom::Point2D(-14,9));

            const bool isSuccessLin2 = TestWarpGpu(inMIDbl, inMIFlt, wcs2_1, wcs2_2,
                                                  order, interpLen, "[Linear WCS 2]" );

            isSuccess = isSuccess && isSuccessLin1 && isSuccessLin2;
        }
    }
    return isSuccess;
}

bool GpuTestExceptions(const afwImage::MaskedImage<double>& inImg)
{
    lsst::pex::logging::Trace::setVerbosity("lsst.afw.kernel", 5);

    afwImage::MaskedImage<double> resImg(inImg.getDimensions());

    bool isSuccess = true;

    PrintSeparator();

    lsst::afw::gpu::DevicePreference devPrefUGpu = lsst::afw::gpu::USE_GPU;
    lsst::afw::gpu::DevicePreference devPrefUCpu = lsst::afw::gpu::USE_CPU;
    lsst::afw::gpu::DevicePreference devPrefAuto = lsst::afw::gpu::AUTO;
    lsst::afw::gpu::DevicePreference devPrefSafe = lsst::afw::gpu::AUTO_WITH_CPU_FALLBACK;

    afwMath::LanczosWarpingKernel lanKernel(2);
    afwMath::BilinearWarpingKernel bilKernel;

    afwImage::Wcs::Ptr wcs1=GetLinWcs(1  ,  -1, -0.5,  1  );
    afwImage::Wcs::Ptr wcs2=GetLinWcs(2.2, 1.3,  3.4, -1.1);

    cout << endl;
    bool isThrown;
    isThrown = false;
    try {
        afwMath::warpImage(resImg, *wcs1, inImg, *wcs2, lanKernel, defaultInterpLen, GetEdgePixel(resImg), devPrefAuto);
    } catch(pexEx::Exception) {
        isThrown = true;
    }

    if (isThrown) {
        isSuccess = false;
        cout << "ERROR: GPU warping with Lanczos kernel with AUTO "
             << "should have not thrown an exception" << endl;
    }

    isThrown = false;
    try {
        afwMath::warpImage(resImg, *wcs1, inImg, *wcs2, bilKernel, defaultInterpLen, GetEdgePixel(resImg), devPrefUGpu);
    } catch(pexEx::Exception) {
        isThrown = true;
    }

    if (lsst::afw::gpu::isGpuEnabled() && !isThrown) {
        isSuccess = false;
        cout << "ERROR: GPU warping with bilinear kernel with USE_GPU"
             << "should have thrown an exception because acceleration of bilinear kernel is not supported" << endl;
    }

    isThrown = false;
    try {
        afwMath::warpImage(resImg, *wcs1, inImg, *wcs2, bilKernel, defaultInterpLen, GetEdgePixel(resImg), devPrefSafe);
    } catch(pexEx::Exception) {
        isThrown = true;
    }

    if (isThrown) {
        isSuccess = false;
        cout << "ERROR: GPU warping with bilinear function kernel with AUTO_WITH_CPU_FALLBACK"
             << "should not have thrown an exception because it should have fell back to CPU code path" << endl;
    }

    isThrown = false;
    try {
        afwMath::warpImage(resImg, *wcs1, inImg, *wcs2, lanKernel, 0, GetEdgePixel(resImg), devPrefAuto);
    } catch(pexEx::Exception) {
        isThrown = true;
    }

    if (isThrown) {
        isSuccess = false;
        cout << "ERROR: GPU warping with no interpolation with AUTO"
             << "should not have thrown an exception because it should have used CPU code path" << endl;
    }

    isThrown = false;
    try {
        afwMath::warpImage(resImg, *wcs1, inImg, *wcs2, lanKernel, 0, GetEdgePixel(resImg), devPrefUCpu);
    } catch(pexEx::Exception) {
        isThrown = true;
    }

    if (isThrown) {
        isSuccess = false;
        cout << "ERROR: GPU warping with no interpolation kernel with USE_CPU"
             << "should not have thrown an exception because it should have used CPU code path" << endl;
    }

    isThrown = false;
    try {
        afwMath::warpImage(resImg, *wcs1, inImg, *wcs2, lanKernel, defaultInterpLen, GetEdgePixel(resImg), devPrefUGpu);
    } catch(pexEx::Exception) {
        isThrown = true;
    }

    if (isThrown) {
        isSuccess = false;
        cout << "ERROR: GPU warping with Lanczos kernel with USE_GPU "
             << "should not have thrown an exception" << endl;
    }

    isThrown = false;
    try {
        afwMath::warpImage(resImg, *wcs1, inImg, *wcs2, lanKernel, 0, GetEdgePixel(resImg), devPrefUGpu);
    } catch(pexEx::Exception) {
        isThrown = true;
    }

    if (lsst::afw::gpu::isGpuEnabled() && !isThrown) {
        isSuccess = false;
        cout << "ERROR: GPU warping with no interpolation "
             << "with Lanczos kernel with USE_GPU "
             << "should have thrown an exception because interpolation is obligatory for GPU acceleration" << endl;
    }

    isThrown = false;
    try {
        afwMath::warpImage(resImg, *wcs1, inImg, *wcs2, bilKernel, defaultInterpLen, GetEdgePixel(resImg), devPrefAuto);
    } catch(pexEx::Exception) {
        isThrown = true;
    }

    if (isThrown) {
        isSuccess = false;
        cout << "ERROR: GPU warping with bilinear kernel with AUTO "
             << "should not have thrown an exception because it should have used CPU code path" << endl;
    }

    if (isSuccess) {
        cout << "All GPU exception tests passed." << endl;
    }

    cout << endl;

    return isSuccess;
}

bool CpuTestExceptions(const afwImage::MaskedImage<double>& inImg)
{
    lsst::pex::logging::Trace::setVerbosity("lsst.afw.kernel", 5);

    afwImage::MaskedImage<double> resImg(inImg.getDimensions());

    bool isSuccess = true;

    PrintSeparator();

    lsst::afw::gpu::DevicePreference devPrefUGpu = lsst::afw::gpu::USE_GPU;
    lsst::afw::gpu::DevicePreference devPrefUCpu = lsst::afw::gpu::USE_CPU;
    lsst::afw::gpu::DevicePreference devPrefAuto = lsst::afw::gpu::AUTO;
    lsst::afw::gpu::DevicePreference devPrefSafe = lsst::afw::gpu::AUTO_WITH_CPU_FALLBACK;

    afwMath::LanczosWarpingKernel lanKernel(2);
    afwMath::BilinearWarpingKernel bilKernel;

    afwImage::Wcs::Ptr wcs1=GetLinWcs(1  ,  -1, -0.5,  1  );
    afwImage::Wcs::Ptr wcs2=GetLinWcs(2.2, 1.3,  3.4, -1.1);

    cout << endl;
    bool isThrown;
    isThrown = false;
    try {
        afwMath::warpImage(resImg, *wcs1, inImg, *wcs2, lanKernel, defaultInterpLen, GetEdgePixel(resImg), devPrefAuto);
    } catch(pexEx::Exception) {
        isThrown = true;
    }

    if (isThrown) {
        isSuccess = false;
        cout << "ERROR: GPU warping with Lanczos kernel with AUTO "
             << "should have not thrown an exception" << endl;
    }

    isThrown = false;
    try {
        afwMath::warpImage(resImg, *wcs1, inImg, *wcs2, lanKernel, defaultInterpLen, GetEdgePixel(resImg), devPrefUGpu);
    } catch(pexEx::Exception) {
        isThrown = true;
    }

    if (lsst::afw::gpu::isGpuEnabled() && !isThrown) {
        isSuccess = false;
        cout << "ERROR: GPU warping with Lanczos kernel with USE_GPU"
             << "should have thrown an exception because AFW was not compiled with GPU support" << endl;
    }

    isThrown = false;
    try {
        afwMath::warpImage(resImg, *wcs1, inImg, *wcs2, lanKernel, 0, GetEdgePixel(resImg), devPrefSafe);
    } catch(pexEx::Exception) {
        isThrown = true;
    }

    if (isThrown) {
        isSuccess = false;
        cout << "ERROR: GPU warping with Lanczos kernel with AUTO_WITH_CPU_FALLBACK"
             << "should not have thrown an exception because it should have used CPU code path" << endl;
    }

    isThrown = false;
    try {
        afwMath::warpImage(resImg, *wcs1, inImg, *wcs2, lanKernel, 0, GetEdgePixel(resImg), devPrefAuto);
    } catch(pexEx::Exception) {
        isThrown = true;
    }

    if (isThrown) {
        isSuccess = false;
        cout << "ERROR: GPU warping with Lanczos kernel with AUTO"
             << "should not have thrown an exception because it should have used CPU code path" << endl;
    }

    isThrown = false;
    try {
        afwMath::warpImage(resImg, *wcs1, inImg, *wcs2, lanKernel, 0, GetEdgePixel(resImg), devPrefUCpu);
    } catch(pexEx::Exception) {
        isThrown = true;
    }

    if (isThrown) {
        isSuccess = false;
        cout << "ERROR: GPU warping with Lanczos kernel with USE_CPU"
             << "should not have thrown an exception because it should have used CPU code path" << endl;
    }

    isThrown = false;
    try {
        afwMath::warpImage(resImg, *wcs1, inImg, *wcs2, lanKernel, 0, GetEdgePixel(resImg), devPrefUGpu);
    } catch(pexEx::Exception) {
        isThrown = true;
    }

    if (lsst::afw::gpu::isGpuEnabled() && !isThrown) {
        isSuccess = false;
        cout << "ERROR: GPU warping with Lanczos kernel with USE_GPU "
             << "should have thrown an exception because AFW was not compiled with GPU support" << endl;
    }

    isThrown = false;
    try {
        afwMath::warpImage(resImg, *wcs1, inImg, *wcs2, bilKernel, 2, GetEdgePixel(resImg), devPrefUGpu);
    } catch(pexEx::Exception) {
        isThrown = true;
    }

    if (lsst::afw::gpu::isGpuEnabled() && !isThrown) {
        isSuccess = false;
        cout << "ERROR: GPU warping with bilinear kernel with USE_GPU "
             << "should have thrown an exception because AFW was not compiled with GPU support" << endl;
    }

    isThrown = false;
    try {
        afwMath::warpImage(resImg, *wcs1, inImg, *wcs2, bilKernel, 2, GetEdgePixel(resImg), devPrefAuto);
    } catch(pexEx::Exception) {
        isThrown = true;
    }

    if (isThrown) {
        isSuccess = false;
        cout << "ERROR: GPU warping with bilinear kernel with AUTO "
             << "should not have thrown an exception because it should have used CPU code path" << endl;
    }

    if (isSuccess) {
        cout << "All GPU exception tests passed (but AFW was not compiled with GPU support!!)." << endl;
        cout << "Additional tests will be performed when GPU acceleration is available." << endl;
    }
    cout << endl;

    return isSuccess;
}

TestResult TestGpu(int argc, char**argv)
{
    lsst::afw::gpu::detail::PrintCudaDeviceInfo();
    string baseFileName = GetInputFileName(argc, argv);

    const afwImage::MaskedImage<float>    inImgFlt(baseFileName);
    const afwImage::MaskedImage<double>   inImgDbl(baseFileName);

    const bool isSuccess1 = GpuTestAccuracy(inImgDbl, inImgFlt);
    const bool isSuccess2 = GpuTestExceptions(inImgDbl);
    const bool isSuccess = isSuccess1 && isSuccess2;

    return isSuccess ? EXIT_SUCCESS : EXIT_FAILURE;
}

TestResult TestCpu(int argc, char**argv)
{
    string baseFileName = GetInputFileName(argc, argv);
    const afwImage::MaskedImage<double>   inImgDbl(baseFileName);

    const bool isSuccess = CpuTestExceptions(inImgDbl);

    return isSuccess ? EXIT_SUCCESS : EXIT_FAILURE;
}


int main(int argc, char **argv)
{
    int status = EXIT_SUCCESS;

    PrintSeparator();
    cout << endl;
    cout << "Note: Dev =  coefficient of variation of RMSD" << endl;
    cout << "Note: Interpolation length set to " << defaultInterpLen << endl;
    cout << endl;

    try {
        if (lsst::afw::gpu::isGpuBuild()) {
            status = TestGpu(argc, argv);
        } else {
            status = TestCpu(argc, argv);
        }
    } catch (pexEx::Exception &e) {
        clog << e.what() << endl;
        status = EXIT_FAILURE;
    }

    PrintSeparator();
    cout << endl;
    if (status == EXIT_FAILURE) {
        cout << "Some tests have failed." << endl;
    } else {
        cout << "All tests passed OK." << endl;
    }

    cout << endl;

    // Check for memory leaks
    if (lsst::daf::base::Citizen::census(0) == 0) {
        cerr << "No leaks detected" << endl;
    } else {
        cerr << "Leaked memory blocks:" << endl;
        lsst::daf::base::Citizen::census(cerr);
        status = EXIT_FAILURE;
    }
    return status;
}

