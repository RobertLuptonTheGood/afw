%define fitsLib_DOCSTRING
"
Python interface to lsst::afw::fits exception classes
"
%enddef

%feature("autodoc", "1");
%module(package="lsst.afw.fits", docstring=fitsLib_DOCSTRING) fitsLib

%{
#include "lsst/afw/fits.h"
#include "lsst/pex/exceptions.h"
%}

%include "lsst/p_lsstSwig.i"

%lsst_exceptions();

%pythonnondynamic;
%naturalvar;  // use const reference typemaps

%import "lsst/pex/exceptions/exceptionsLib.i"

%include "lsst/afw/fits.h"

%types(lsst::afw::fits::FitsError *);
%types(lsst::afw::fits::FitsTypeError *);
