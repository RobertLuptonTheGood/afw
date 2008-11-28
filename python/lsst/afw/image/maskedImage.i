// -*- lsst-c++ -*-
%{
#   include "lsst/afw/image/MaskedImage.h"
%}

/************************************************************************************************************/
//
// Must go Before the %include
//
%define %maskedImagePtr(NAME, TYPE, PIXEL_TYPES...)
SWIG_SHARED_PTR_DERIVED(NAME##TYPE, lsst::daf::data::LsstBase, lsst::afw::image::MaskedImage<PIXEL_TYPES>);
%enddef

//
// Must go After the %include
//
%define %maskedImage(NAME, TYPE, PIXEL_TYPES...)
%template(NAME##TYPE) lsst::afw::image::MaskedImage<PIXEL_TYPES>;
%lsst_persistable(lsst::afw::image::MaskedImage<PIXEL_TYPES>);

%extend lsst::afw::image::MaskedImage<PIXEL_TYPES> {
    %pythoncode {
    def Factory(self, *args):
        """Return a MaskedImage of this type"""
        return NAME##TYPE(*args)

    def set(self, x, y=None, values=None):
        """Set the point (x, y) to a triple (value, mask, variance)"""

        if values is None:
            assert (y is None)
            values = x
            try:
                self.getImage().set(values[0])
                self.getMask().set(values[1])
                self.getVariance().set(values[2])
            except TypeError:
                self.getImage().set(values)
                self.getMask().set(0)
                self.getVariance().set(0)
        else:
            try:
                self.getImage().set(x, y, values[0])
                self.getMask().set(x, y, values[1])
                self.getVariance().set(x, y, values[2])
            except TypeError:
                self.getImage().set(x)
                self.getMask().set(y)
                self.getVariance().set(values)

    def get(self, x, y):
        """Return a triple (value, mask, variance) at the point (x, y)"""
        return (self.getImage().get(x, y),
                self.getMask().get(x, y),
                self.getVariance().get(x, y))

    #
    # Deal with incorrect swig wrappers for C++ "void operator op=()"
    #
    def __ilshift__(*args):
        """__ilshift__(self, NAME##TYPE src) -> self"""
        _imageLib.MaskedImage##TYPE##___ilshift__(*args)
        return args[0]

    def __ior__(*args):
        """__ior__(self, NAME##TYPE src) -> self"""
        _imageLib.NAME##TYPE##___ior__(*args)
        return args[0]

    def __iand__(*args):
        """__iand__(self, NAME##TYPE src) -> self"""
        _imageLib.NAME##TYPE##___iand__(*args)
        return args[0]

    def __iadd__(*args):
        """
        __iadd__(self, float scalar) -> self
        __iadd__(self, NAME##TYPE inputImage) -> self
        """
        _imageLib.NAME##TYPE##___iadd__(*args)
        return args[0]

    def __isub__(*args):
        """
        __isub__(self, float scalar)
        __isub__(self, NAME##TYPE inputImage)
        """
        _imageLib.NAME##TYPE##___isub__(*args)
        return args[0]
    

    def __imul__(*args):
        """
        __imul__(self, float scalar)
        __imul__(self, NAME##TYPE inputImage)
        """
        _imageLib.NAME##TYPE##___imul__(*args)
        return args[0]

    def __idiv__(*args):
        """
        __idiv__(self, float scalar)
        __idiv__(self, NAME##TYPE inputImage)
        """
        _imageLib.NAME##TYPE##___idiv__(*args)
        return args[0]
    }
}
%enddef

/************************************************************************************************************/

%ignore lsst::afw::image::MaskedImage::operator();
%ignore lsst::afw::image::MaskedImage::begin;
%ignore lsst::afw::image::MaskedImage::end;
%ignore lsst::afw::image::MaskedImage::at;
%ignore lsst::afw::image::MaskedImage::rbegin;
%ignore lsst::afw::image::MaskedImage::rend;
%ignore lsst::afw::image::MaskedImage::col_begin;
%ignore lsst::afw::image::MaskedImage::col_end;
%ignore lsst::afw::image::MaskedImage::y_at;
%ignore lsst::afw::image::MaskedImage::row_begin;
%ignore lsst::afw::image::MaskedImage::row_end;
%ignore lsst::afw::image::MaskedImage::x_at;
%ignore lsst::afw::image::MaskedImage::xy_at;

%maskedImagePtr(MaskedImage, F, float,  lsst::afw::image::MaskPixel, lsst::afw::image::VariancePixel);
%maskedImagePtr(MaskedImage, D, double, lsst::afw::image::MaskPixel, lsst::afw::image::VariancePixel);

%include "lsst/afw/image/MaskedImage.h"

%maskedImage(MaskedImage, D, double,  lsst::afw::image::MaskPixel, lsst::afw::image::VariancePixel);
%maskedImage(MaskedImage, F, float,  lsst::afw::image::MaskPixel, lsst::afw::image::VariancePixel);