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
 
//
/**
 * \file
 *
 * Classes to support calibration (e.g. photometric zero points, exposure times)
 */
#ifndef LSST_AFW_IMAGE_CALIB_H
#define LSST_AFW_IMAGE_CALIB_H

#include <utility>
#include "boost/shared_ptr.hpp"
#include "lsst/base.h"
#include "lsst/daf/base/DateTime.h"
#include "lsst/afw/geom/Point.h"

namespace lsst {
namespace daf {
    namespace base {
        class PropertySet;
    }
}

namespace afw {
namespace cameraGeom {
    class Detector;
}
namespace image {
/**
 * Describe an exposure's calibration
 */

class Calib
{
public :
    typedef boost::shared_ptr<Calib> Ptr;
    typedef boost::shared_ptr<Calib const> ConstPtr;

    explicit Calib();
    explicit Calib(std::vector<CONST_PTR(Calib)> const& calibs);
    explicit Calib(CONST_PTR(lsst::daf::base::PropertySet));

    void setMidTime(lsst::daf::base::DateTime const& midTime);
    lsst::daf::base::DateTime getMidTime () const;
    lsst::daf::base::DateTime getMidTime(boost::shared_ptr<const lsst::afw::cameraGeom::Detector>,
                                         lsst::afw::geom::Point2I const&) const;

    void setExptime(double exptime);
    double getExptime() const;

    void setFluxMag0(double fluxMag0, double fluxMag0Sigma=0.0);
    void setFluxMag0(std::pair<double, double> fluxMag0AndSigma);
    std::pair<double, double> getFluxMag0() const;

    double getFlux(double const mag) const;
    std::pair<double, double> getFlux(double const mag, double const magErr) const;
    std::vector<double> getFlux(std::vector<double> const& mag) const;
    std::pair<std::vector<double>, std::vector<double> > getFlux(std::vector<double> const& mag,
                                                                 std::vector<double> const& magErr
        ) const;
    double getMagnitude(double const flux) const;
    std::pair<double, double> getMagnitude(double const flux, double const fluxErr) const;
    std::vector<double> getMagnitude(std::vector<double> const& flux) const;
    std::pair<std::vector<double>, std::vector<double> > getMagnitude(std::vector<double> const& flux,
                                                                      std::vector<double> const& fluxErr
        ) const;

    static void setThrowOnNegativeFlux(bool raiseException);
    static bool getThrowOnNegativeFlux();
    /*
     * Compare two Calibs
     */
    bool operator==(Calib const& rhs) const;
    bool operator!=(Calib const& rhs) const { return !(*this == rhs); }
private :
    lsst::daf::base::DateTime _midTime;
    double _exptime;
    double _fluxMag0;
    double _fluxMag0Sigma;
    static bool _throwOnNegativeFlux;
};

namespace detail {
    int stripCalibKeywords(PTR(lsst::daf::base::PropertySet) metadata);
}

}}}  // lsst::afw::image

#endif // LSST_AFW_IMAGE_CALIB_H
