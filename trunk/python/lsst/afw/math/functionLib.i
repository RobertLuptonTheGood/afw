// -*- lsst-c++ -*-
%{
#include "lsst/afw/math/Function.h"
#include "lsst/afw/math/FunctionLibrary.h"
%}

// I'm not sure newobject is needed (the memory leak test works without it)
%newobject lsst::afw::math::Function::getParameters;

%include "lsst/afw/math/Function.h"
%include "lsst/afw/math/FunctionLibrary.h"

%template(FunctionF)          lsst::afw::math::Function<float>;
%template(Function1F)         lsst::afw::math::Function1<float>;
%template(Function2F)         lsst::afw::math::Function2<float>;
%boost_shared_ptr(Function2FPtr,    lsst::afw::math::Function2<float>);

%template(Chebyshev1Function1F) lsst::afw::math::Chebyshev1Function1<float>;
%template(DoubleGaussianFunction2F) lsst::afw::math::DoubleGaussianFunction2<float>;
%template(GaussianFunction1F) lsst::afw::math::GaussianFunction1<float>;
%template(GaussianFunction2F) lsst::afw::math::GaussianFunction2<float>;
%template(IntegerDeltaFunction2F) lsst::afw::math::IntegerDeltaFunction2<float>;
%template(LanczosFunction1F) lsst::afw::math::LanczosFunction1<float>;
%template(LanczosFunction2F) lsst::afw::math::LanczosFunction2<float>;
%template(PolynomialFunction1F) lsst::afw::math::PolynomialFunction1<float>;
%template(PolynomialFunction2F) lsst::afw::math::PolynomialFunction2<float>;

%template(FunctionD)          lsst::afw::math::Function<double>;
%template(Function1D)         lsst::afw::math::Function1<double>;
%template(Function2D)         lsst::afw::math::Function2<double>;
%boost_shared_ptr(Function2DPtr,    lsst::afw::math::Function2<double>);

%template(Chebyshev1Function1D) lsst::afw::math::Chebyshev1Function1<double>;
%template(DoubleGaussianFunction2D) lsst::afw::math::DoubleGaussianFunction2<double>;
%template(GaussianFunction1D) lsst::afw::math::GaussianFunction1<double>;
%template(GaussianFunction2D) lsst::afw::math::GaussianFunction2<double>;
%template(IntegerDeltaFunction2D) lsst::afw::math::IntegerDeltaFunction2<double>;
%template(LanczosFunction1D) lsst::afw::math::LanczosFunction1<double>;
%template(LanczosFunction2D) lsst::afw::math::LanczosFunction2<double>;
%template(PolynomialFunction1D) lsst::afw::math::PolynomialFunction1<double>;
%template(PolynomialFunction2D) lsst::afw::math::PolynomialFunction2<double>;