// -*- lsst-c++ -*-
#include <stdexcept>

#include <lsst/daf/base.h>
#include <lsst/pex/exceptions.h>
#include <lsst/pex/logging/Trace.h>
#include <lsst/afw/image.h>

using namespace std;
using boost::any_cast;

using lsst::pex::logging::Trace;
using lsst::daf::base::DataProperty;

namespace pexEx = lsst::pex::exceptions;
namespace afwImage  = lsst::afw::image;

template <typename MaskPixelT> class testCrFunc : public afwImage::MaskPixelBooleanFunc<MaskPixelT> {
public:
    typedef typename afwImage::Mask<MaskPixelT>::MaskChannelT MaskChannelT;
    testCrFunc(afwImage::Mask<MaskPixelT>& m) : afwImage::MaskPixelBooleanFunc<MaskPixelT>(m) {}
    void init() {
       afwImage::MaskPixelBooleanFunc<MaskPixelT>::_mask.getPlaneBitMask("CR", bitsCR);
    }        
    bool operator ()(MaskPixelT pixel) const { 
       return ((pixel & bitsCR) !=0 ); 
    }
private:
    MaskChannelT bitsCR;
};

/*
 * Make this a subroutine so that locals go out of scope as part of test
 * of memory management
 */
void test() {
    Trace::setDestination(cerr);

    Trace::setVerbosity(".", 100);

// ------------- Test constructors
    typedef uint8 MaskPixelType;
    typedef afwImage::Mask<MaskPixelType>::MaskIVwT MaskImageType;
    typedef afwImage::Mask<MaskPixelType>::MaskIVwPtrT MaskImagePtrType;
    typedef afwImage::Mask<MaskPixelType>::MaskPtrT MaskPtrType;
    
    MaskImagePtrType maskImage(new MaskImageType(300,400));
    cout << maskImage.use_count() << endl;

    afwImage::Mask<MaskPixelType> testMask(maskImage);
    cout << maskImage.use_count() << endl;

    typedef uint16 MaskPixelType2;
    typedef afwImage::Mask<MaskPixelType2>::MaskIVwT MaskImageType2;
    typedef afwImage::Mask<MaskPixelType2>::MaskIVwPtrT MaskImagePtrType2;

    MaskImagePtrType2 maskImage2(new MaskImageType2(300,400));

    afwImage::Mask<MaskPixelType2> testMask2(maskImage2);

    afwImage::Mask<MaskPixelType> testMask3(300,400);

// ------------- Test copy constructor and operator= for memory leaks

    afwImage::Mask<MaskPixelType> maskCopy(testMask3);
    maskCopy = testMask3;

// ------------- Test mask plane addition

    int iPlane;

    try {
       iPlane = testMask.addMaskPlane("CR");
       cout << "Assigned CR to plane " << iPlane << endl;
    } catch(afwImage::OutOfPlaneSpace &e){
       DataProperty::PtrType  propertyList = e.getLast();
       cout << propertyList->toString("",true) << endl;
       DataProperty::PtrType aProperty = propertyList->findUnique("numPlanesUsed");
       int numPlanesUsed = any_cast<const int>(aProperty->getValue());
       DataProperty::PtrType bProperty = propertyList->findUnique("numPlanesMax");
       int numPlanesMax = any_cast<const int>(bProperty->getValue());

       cout << "Ran out of space to add new CR plane: number of Planes: " \
            << numPlanesUsed << "  max Planes: " << numPlanesMax << endl;
       throw;
    }

    try {
        iPlane = testMask.addMaskPlane("BP");
        cout << "Assigned BP to plane " << iPlane << endl;
    } catch(exception &e){
        cout << e.what() << "Ran out of space to add new BP plane" << endl;
        throw;
    }

    for (int i = 0; i <= 8; i++) {
        string sp = (boost::format("P%d") % i).str();
        try {
            cout << boost::format("Assigned %s to plane %d\n") %
                sp % testMask.addMaskPlane(sp);
        } catch(afwImage::OutOfPlaneSpace &e) {
            e.getStack()->toString("\t",true);
        }
    }

    for (int i = 0; i <= 8; i++) {
        string sp = (boost::format("P%d") % i).str();
        try {
            testMask.removeMaskPlane(sp);
        } catch(afwImage::NoMaskPlane) {
            ;
        }
    }

/******************************************************************************/
    
    int planeCR, planeBP;

    try {
        testMask.getMaskPlane("CR", planeCR); 
        cout << "CR plane is " << planeCR << endl;
    } catch(afwImage::NoMaskPlane &e) {
	  cout << e.what() << "No CR plane found" << endl;
         throw;
    }

    try {
        testMask.getMaskPlane("BP", planeBP);
        cout << "BP plane is " << planeBP << endl;
    }
    catch(afwImage::NoMaskPlane &e) {
	  cout << e.what() << "No BP plane found" << endl;
         throw;
    } 

// ------------ Test mask plane metaData

    DataProperty::PtrType metaData = DataProperty::createPropertyNode("testMetaData");
    testMask.addMaskPlaneMetaData(metaData);
    cout << "MaskPlane metadata:" << endl;
    cout << metaData->toString("\t",true) << endl;

    DataProperty::PtrType newPlane(new DataProperty("Whatever", 5));
    metaData->addProperty(newPlane);

    testMask.parseMaskPlaneMetaData(metaData);
    cout << "After loading metadata: " << endl;
    testMask.printMaskPlanes();

// ------------ Test mask plane operations

    testMask.clearMaskPlane(planeCR);

    PixelCoord coord;
    list<PixelCoord> pixelList;

    for (int x=0; x<300; x+=1) {
	  for (int y=300; y<400; y+=20) {
	       coord.x = x;
	       coord.y = y;
	       pixelList.push_back(coord);
	  }
    }

    testMask.setMaskPlaneValues(planeCR, pixelList);

    testMask.setMaskPlaneValues(planeBP, pixelList);

    for (int x=250; x<300; x+=10) {
	  for (int y=300; y<400; y+=20) {
	       cout << x << " " << y << " " << (int)testMask(x, y) << " " << testMask(x, y, planeCR) << endl;
	  }
    }

    testMask.clearMaskPlane(planeCR);

    cout << endl;
    for (int x=250; x<300; x+=10) {
	  for (int y=300; y<400; y+=20) {
	       cout << x << " " << y << " " << (int)testMask(x, y) << " " << testMask(x, y, planeCR) << endl;
	  }
    }

// ------------------ Test |= operator
   
    try {
        iPlane = testMask3.addMaskPlane("CR");
        cout << "Assigned CR to plane " << iPlane << endl;
    } catch(exception &e){
        cout << e.what() << "Ran out of space to add new CR plane" << endl;
        throw;
    }

    testMask |= testMask3;

    cout << "Applied |= operator" << endl;
    
// -------------- Test mask plane removal

    testMask.clearMaskPlane(planeBP);
    testMask.removeMaskPlane("BP");

    try {
        testMask.getMaskPlane("CR", planeCR);
    } catch(afwImage::NoMaskPlane &e) {
	  cout << e.what() << "No CR plane found" << endl;
         throw;
    } 
    cout << "CR plane is " << planeCR << endl;

    try {
        testMask.getMaskPlane("BP", planeBP);
        cout << "BP plane is " << planeBP << endl;
    } catch(afwImage::NoMaskPlane &e) {
	  cout << e.what() << "No BP plane found" << endl;
         // testing success of plane deletion so NO  throw;
    } 

// --------------- Test submask methods

    testMask.setMaskPlaneValues(planeCR, pixelList);

    vw::BBox2i region(100, 300, 10, 40);
    MaskPtrType subTestMask = testMask.getSubMask(region);

    testMask.clearMaskPlane(planeCR);

    testMask.replaceSubMask(region, subTestMask);

    cout << endl;
    for (int x=90; x<120; x+=1) {
	  for (int y=295; y<350; y+=5) {
	       cout << x << " " << y << " " << (int)testMask(x, y) << " " << testMask(x, y, planeCR) << endl;
	  }
    }


// --------------------- Test MaskPixelBooleanFunc

    testCrFunc<MaskPixelType> testCrFuncInstance(testMask);

    // Calling init gets the latest plane info from testMask
    testCrFuncInstance.init();

    // Now use testCrFuncInstance
    int count = testMask.countMask(testCrFuncInstance, region);
    cout << count << " pixels had CR set in region" << endl;

    // This should generate a vw exception - dimensions of region and submask must be =

    cout << "This should throw an exception:" << endl;

    try {
       region.expand(10);
       testMask.replaceSubMask(region, subTestMask);
    } catch (pexEx::ExceptionStack &e) {
        cout << "Exception handler: Caught the buggy code: " << e.what() << endl;
    } catch (exception eex) {
        // Catch base STD exception (from which all exceptions should be derived)
       cout << "Exception handler (exception eex): Caught the buggy code" << endl;
    } catch (...) {
        // Catch all exceptions not derived from STD base exception
        cout << "Exception handler (...): Caught the buggy code" << endl;
    }
}

#if 0
    //----------------------------------------------------------------
    // Following exceptions are the only exceptions currently used in VW. 
    //     Additional VW exceptions are defined in vw:Exception.h
    //     VW exceptions are derived from std:exception

    catch (vw::NoImplErr noirex) {
       cout << "Exception handler (vw::NoImplErr noirex): Caught the buggy code" << endl;
    }
    catch (vw::LogicErr lrex) {
       cout << "Exception handler (vw::LoginErr lrex): Caught the buggy code" << endl;
    }
    catch (vw::IOErr iorex) {
       cout << "Exception handler (vw::IOErr iorex): Caught the buggy code" << endl;
    }
    catch (vw::ArgumentErr vwrex) {
       cout << "Exception handler (vw::ArgumentErr vwrex): Caught the buggy code" << endl;
    }
#endif

int main(int argc, char *argv[]) {
    try {
       try {
           test();
       } catch (pexEx::ExceptionStack &e) {
           throw pexEx::Runtime(string("In handler\n") + e.what());
       }
    } catch (pexEx::ExceptionStack &e) {
       clog << e.what() << endl;
    }

    //
    // Check for memory leaks
    //
    if (lsst::daf::base::Citizen::census(0) == 0) {
        cerr << "No leaks detected" << endl;
    } else {
        cerr << "Leaked memory blocks:" << endl;
        lsst::daf::base::Citizen::census(cerr);
    }
}