// -*- LSST-C++ -*-
//! \file
//! \brief Provides support for file formats via libJPEG.

#ifndef LSST_AFW_IMAGE_DISKIMAGERESOURCESFITS_H
#define LSST_AFW_IMAGE_DISKIMAGERESOURCESFITS_H

#include <string>

#include <vw/Math/BBox.h>
#include <vw/Image/PixelTypes.h>
#include <vw/Image/ImageResource.h>
#include <vw/FileIO/DiskImageResource.h>

#include <lsst/daf/base/DataProperty.h>
// no such file: #include <lsst/pex/utils/Utils.h>

namespace lsst {
namespace afw {
namespace image {

/*!
 * FITS I/O for VisionWorkbench
 */
class DiskImageResourceFITS : public vw::DiskImageResource {
public:
    explicit DiskImageResourceFITS();
    DiskImageResourceFITS(std::string const& filename);
    DiskImageResourceFITS(std::string const& filename, 
                          vw::ImageFormat const& format);
    
    virtual ~DiskImageResourceFITS();
    
    virtual void read(vw::ImageBuffer const& dest, vw::BBox2i const& bbox) const;
    virtual void write(vw::ImageBuffer const& dest, vw::BBox2i const& bbox);
    virtual void flush();
    
    void open(std::string const& filename);
    
    void create(std::string const& filename,
                vw::ImageFormat const& format,
                lsst::daf::base::DataProperty *metaData = NULL);
    
    static DiskImageResource* construct_open(std::string const& filename);
    
    static DiskImageResource* construct_create(std::string const& filename,
                                               vw::ImageFormat const& format);
    // Accessors
    const int getHdu() const { return _hdu; }
    void setHdu(const int hdu) { _hdu = hdu; }
    int getNumKeys();
    bool getKey(int n, std::string & keyWord, std::string & keyValue, std::string & keyComment);
    bool appendKey(const std::string & keyWord, const boost::any & keyValue, const std::string & keyComment);
private:
    static bool _typeIsRegistered;   //!< Have we registered our file suffixes with VW?
    static int _defaultHdu;          //!< Default HDU to use when opening files
    static void setDefaultHdu(const int hdu);
    
    std::string _filename;          //!< filename
    lsst::daf::base::DataProperty *_metaData;        //!< metadata to write to the file
    void *_fd;                      //!< really a fitsfile, but we aren't including cfitsio.h
    int _hdu;                       //!< desired HDU
    int _ttype;                     //!< cfitsio's name for data type
    int _bitpix;                    //!< FITS' BITPIX keyword
    vw::ChannelTypeEnum _channelType; //!< VW's name for data type
};

//! Free function to read a FITS image on disk into a vw::ImageView<T> object.
//
//! We need this routine so as to:
//!   1. be able to read FITS files with arbitrary names
//!   2. be able to control which HDU we read
template <typename PixelT>
void read(vw::ImageView<PixelT>& image, //!< Desired image
          std::string const& filename, //!< Resource to read (e.g. filename)
          const int hdu = 0             //!< Desired HDU; 0 => PDU
         ) {
    // Open the file for reading.  We call DiskImageResourceFITS::construct_open
    // directly to bypass the need to register our file suffixes (although we
    // do that too)
        
    DiskImageResourceFITS *r =
        dynamic_cast<DiskImageResourceFITS *>(DiskImageResourceFITS::construct_open(filename));
    r->setHdu(hdu);                 // Currently, only open knows about HDUs
    read_image(image, *r);
    
    delete r;
}

}}} // lsst::afw::image

#endif // LSST_AFW_IMAGE_DISKIMAGERESOURCESFITS_H