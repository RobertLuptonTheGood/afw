changecom(`###')dnl
// -*- lsst-c++ -*-
/* 
 * LSST Data Management System
 * Copyright 2008, 2009, 2010, 2011 LSST Corporation.
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

// THIS FILE IS AUTOMATICALLY GENERATED, AND WILL BE OVERWRITTEN IF EDITED MANUALLY.

define(`m4def', defn(`define'))dnl
m4def(`DECLARE_SLOT_GETTERS',
`/// @brief Get the value of the $1$2 slot measurement.
    $2::MeasValue get$1$2() const;

    /// @brief Get the uncertainty on the $1$2 slot measurement.
    $2::ErrValue get$1$2Err() const;

    /// @brief Return true if the measurement in the $1$2 slot was successful.
    bool get$1$2Flag() const;
')dnl
m4def(`DECLARE_FLUX_GETTERS', `DECLARE_SLOT_GETTERS($1, `Flux')')dnl
m4def(`DECLARE_CENTROID_GETTERS', `DECLARE_SLOT_GETTERS(`', `Centroid')')dnl
m4def(`DECLARE_SHAPE_GETTERS', `DECLARE_SLOT_GETTERS(`', `Shape')')dnl
m4def(`DEFINE_SLOT_GETTERS',
`inline $2::MeasValue SourceRecord::get$1$2() const {
    return this->get(getTable()->get$1$2Key());
}

inline $2::ErrValue SourceRecord::get$1$2Err() const {
    return this->get(getTable()->get$1$2ErrKey());
}

inline bool SourceRecord::get$1$2Flag() const {
    return this->get(getTable()->get$1$2FlagKey());
}
')dnl
m4def(`DEFINE_FLUX_GETTERS', `DEFINE_SLOT_GETTERS($1, `Flux')')dnl
m4def(`DEFINE_CENTROID_GETTERS', `DEFINE_SLOT_GETTERS(`', `Centroid')')dnl
m4def(`DEFINE_SHAPE_GETTERS', `DEFINE_SLOT_GETTERS(`', `Shape')')dnl
m4def(`DECLARE_SLOT_DEFINERS',
`/**
     * @brief Set the measurement used for the $1$2 slot using Keys.
     */
    void define$1$2($2::MeasKey const & meas, $2::ErrKey const & err, Key<Flag> const & flag) {
        _slot$2$3 = KeyTuple<$2>(meas, err, flag);
    }

    /**
     *  @brief Set the measurement used for the $1$2 slot with a field name.
     *
     *  This requires that the measurement adhere to the convention of having
     *  "<name>", "<name>.err", and "<name>.flags" fields.
     */
    void define$1$2(std::string const & name) {
        Schema schema = getSchema();
        _slot$2$3 = KeyTuple<$2>(schema[name], schema[name]["err"], schema[name]["flags"]);
    }

    /// @brief Return the name of the field used for the $1$2 slot.
    std::string get$1$2Definition() const {
        return getSchema().find(_slot$2$3.meas).field.getName();
    }

    /// @brief Return the key used for the $1$2 slot.
    $2::MeasKey get$1$2Key() const { return _slot$2$3.meas; }

    /// @brief Return the key used for $1$2 slot error or covariance.
    $2::ErrKey get$1$2ErrKey() const { return _slot$2$3.err; }

    /// @brief Return the key used for the $1$2 slot success flag.
    Key<Flag> get$1$2FlagKey() const { return _slot$2$3.flag; }
')dnl
m4def(`DECLARE_FLUX_DEFINERS', `DECLARE_SLOT_DEFINERS($1, `Flux', `[FLUX_SLOT_`'translit($1, `a-z', `A-Z')]')')dnl
m4def(`DECLARE_CENTROID_DEFINERS', `DECLARE_SLOT_DEFINERS(`', `Centroid', `')')dnl
m4def(`DECLARE_SHAPE_DEFINERS', `DECLARE_SLOT_DEFINERS(`', `Shape', `')')dnl
#ifndef AFW_TABLE_Source_h_INCLUDED
#define AFW_TABLE_Source_h_INCLUDED

#include "boost/array.hpp"
#include "boost/type_traits/is_convertible.hpp"

#include "lsst/afw/detection/Footprint.h"
#include "lsst/afw/table/Simple.h"
#include "lsst/afw/table/IdFactory.h"
#include "lsst/afw/table/Catalog.h"
#include "lsst/afw/table/BaseColumnView.h"
#include "lsst/afw/table/io/FitsWriter.h"

namespace lsst { namespace afw {

namespace image {
class Wcs;
} // namespace image

namespace table {

typedef lsst::afw::detection::Footprint Footprint;

class SourceRecord;
class SourceTable;

/// @brief A collection of types that correspond to common measurements.
template <typename MeasTagT, typename ErrTagT>
struct Measurement {
    typedef MeasTagT MeasTag;  ///< the tag (template parameter) type used for the measurement
    typedef ErrTagT ErrTag;    ///< the tag (template parameter) type used for the uncertainty
    typedef typename Field<MeasTag>::Value MeasValue; ///< the value type used for the measurement
    typedef typename Field<ErrTag>::Value ErrValue;   ///< the value type used for the uncertainty
    typedef Key<MeasTag> MeasKey;  ///< the Key type for the actual measurement
    typedef Key<ErrTag> ErrKey;    ///< the Key type for the error on the measurement
};

#ifndef SWIG

/// A collection of types useful for flux measurement algorithms.
struct Flux : public Measurement<double,double> {};

/// A collection of types useful for centroid measurement algorithms.
struct Centroid : public Measurement< Point<double>, Covariance< Point<double> > > {};

/// A collection of types useful for shape measurement algorithms.
struct Shape : public Measurement< Moments<double>, Covariance< Moments<double> > > {};

/// An enum for all the special flux aliases in Source.
enum FluxSlotEnum {
    FLUX_SLOT_PSF=0,
    FLUX_SLOT_MODEL,
    FLUX_SLOT_AP,
    FLUX_SLOT_INST,
    N_FLUX_SLOTS
};
/**
 *  @brief A three-element tuple of measurement, uncertainty, and flag keys.
 *
 *  Most measurement should have more than one flag key to indicate different kinds of failures.
 *  This flag key should usually be set to be a logical OR of all of them, so it is set whenever
 *  a measurement cannot be fully trusted.
 */
template <typename MeasurementT>
struct KeyTuple {
    typename MeasurementT::MeasKey meas; ///< Key used for the measured value.
    typename MeasurementT::ErrKey err;   ///< Key used for the uncertainty.
    Key<Flag> flag;                      ///< Failure bit; set if the measurement did not fully succeed.

    /// Default-constructor; all keys will be invalid.
    KeyTuple() {}

    /// Main constructor.
    KeyTuple(
        typename MeasurementT::MeasKey const & meas_,
        typename MeasurementT::ErrKey const & err_,
        Key<Flag> const & flag_
    ) : meas(meas_), err(err_), flag(flag_) {}

};

/// Convenience function to setup fields for centroid measurement algorithms.
KeyTuple<Centroid> addCentroidFields(Schema & schema, std::string const & name, std::string const & doc);

/// Convenience function to setup fields for shape measurement algorithms.
KeyTuple<Shape> addShapeFields(Schema & schema, std::string const & name, std::string const & doc);

/// Convenience function to setup fields for flux measurement algorithms.
KeyTuple<Flux> addFluxFields(Schema & schema, std::string const & name, std::string const & doc);

#endif // !SWIG

template <typename RecordT> class SourceColumnViewT;

/**
 *  @brief Record class that contains measurements made on a single exposure.
 *
 *  Sources provide four additions to SimpleRecord / SimpleRecord:
 *   - Specific fields that must always be present, with specialized getters.
 *     The schema for a SourceTable should always be constructed by starting with the result of
 *     SourceTable::makeMinimalSchema.
 *   - A shared_ptr to a Footprint for each record.
 *   - A system of aliases (called slots) in which a SourceTable instance stores keys for particular
 *     measurements (a centroid, a shape, and a number of different fluxes) and SourceRecord uses
 *     this keys to provide custom getters and setters.  These are not separate fields, but rather
 *     aliases that can point to custom fields.
 */
class SourceRecord : public SimpleRecord {
public:

    typedef SourceTable Table;
    typedef SourceColumnViewT<SourceRecord> ColumnView;
    typedef SimpleCatalogT<SourceRecord> Catalog;
    typedef SimpleCatalogT<SourceRecord const> ConstCatalog;

    PTR(Footprint) getFootprint() const { return _footprint; }

    void setFootprint(PTR(Footprint) const & footprint) { _footprint = footprint; }

    CONST_PTR(SourceTable) getTable() const {
        return boost::static_pointer_cast<SourceTable const>(BaseRecord::getTable());
    }

    //@{
    /// @brief Convenience accessors for the keys in the minimal source schema.
    RecordId getParent() const;
    void setParent(RecordId id);
    //@}

    DECLARE_FLUX_GETTERS(`Psf')
    DECLARE_FLUX_GETTERS(`Model')
    DECLARE_FLUX_GETTERS(`Ap')
    DECLARE_FLUX_GETTERS(`Inst')
    DECLARE_CENTROID_GETTERS
    DECLARE_SHAPE_GETTERS

    /// @brief Return the centroid slot x coordinate.
    double getX() const;

    /// @brief Return the centroid slot y coordinate.
    double getY() const;

    /// @brief Return the shape slot Ixx value.
    double getIxx() const;

    /// @brief Return the shape slot Iyy value.
    double getIyy() const;

    /// @brief Return the shape slot Ixy value.
    double getIxy() const;

    /// @brief Update the coord field using the given Wcs and the field in the centroid slot.
    void updateCoord(image::Wcs const & wcs);

    /// @brief Update the coord field using the given Wcs and the image center from the given key.
    void updateCoord(image::Wcs const & wcs, Key< Point<double> > const & key);

protected:

    SourceRecord(PTR(SourceTable) const & table);

    virtual void _assign(BaseRecord const & other);

private:
    PTR(Footprint) _footprint;
};

/**
 *  @brief Table class that contains measurements made on a single exposure.
 *
 *  @copydetails SourceRecord
 */
class SourceTable : public SimpleTable {
public:

    typedef SourceRecord Record;
    typedef SourceColumnViewT<SourceRecord> ColumnView;
    typedef SimpleCatalogT<Record> Catalog;
    typedef SimpleCatalogT<Record const> ConstCatalog;

    /**
     *  @brief Construct a new table.
     *
     *  @param[in] schema            Schema that defines the fields, offsets, and record size for the table.
     *  @param[in] idFactory         Factory class to generate record IDs when they are not explicitly given.
     *                               If null, record IDs will default to zero.
     *
     *  Note that not passing an IdFactory at all will call the other override of make(), which will
     *  set the ID factory to IdFactory::makeSimple().
     */
    static PTR(SourceTable) make(Schema const & schema, PTR(IdFactory) const & idFactory);

    /**
     *  @brief Construct a new table.
     *
     *  @param[in] schema            Schema that defines the fields, offsets, and record size for the table.
     *
     *  This overload sets the ID factory to IdFactory::makeSimple().
     */
    static PTR(SourceTable) make(Schema const & schema) { return make(schema, IdFactory::makeSimple()); }

    /**
     *  @brief Return a minimal schema for Source tables and records.
     *
     *  The returned schema can and generally should be modified further,
     *  but many operations on sources will assume that at least the fields
     *  provided by this routine are present.
     *
     *  Keys for the standard fields added by this routine can be obtained
     *  from other static member functions of the SourceTable class.
     */
    static Schema makeMinimalSchema() { return getMinimalSchema().schema; }

    /**
     *  @brief Return true if the given schema is a valid SourceTable schema.
     *  
     *  This will always be true if the given schema was originally constructed
     *  using makeMinimalSchema(), and will rarely be true otherwise.
     */
    static bool checkSchema(Schema const & other) {
        return other.contains(getMinimalSchema().schema);
    }

    /// @brief Key for the parent ID.
    static Key<RecordId> getParentKey() { return getMinimalSchema().parent; }

    /// @copydoc BaseTable::clone
    PTR(SourceTable) clone() const { return boost::static_pointer_cast<SourceTable>(_clone()); }

    /// @copydoc BaseTable::makeRecord
    PTR(SourceRecord) makeRecord() { return boost::static_pointer_cast<SourceRecord>(_makeRecord()); }

    /// @copydoc BaseTable::copyRecord
    PTR(SourceRecord) copyRecord(BaseRecord const & other) {
        return boost::static_pointer_cast<SourceRecord>(BaseTable::copyRecord(other));
    }

    /// @copydoc BaseTable::copyRecord
    PTR(SourceRecord) copyRecord(BaseRecord const & other, SchemaMapper const & mapper) {
        return boost::static_pointer_cast<SourceRecord>(BaseTable::copyRecord(other, mapper));
    }

    DECLARE_FLUX_DEFINERS(`Psf')
    DECLARE_FLUX_DEFINERS(`Model')
    DECLARE_FLUX_DEFINERS(`Ap')
    DECLARE_FLUX_DEFINERS(`Inst')
    DECLARE_CENTROID_DEFINERS
    DECLARE_SHAPE_DEFINERS
protected:

    SourceTable(Schema const & schema, PTR(IdFactory) const & idFactory);

    SourceTable(SourceTable const & other);

private:

    // Struct that holds the minimal schema and the special keys we've added to it.
    struct MinimalSchema {
        Schema schema;
        Key<RecordId> parent;

        MinimalSchema();
    };
    
    // Return the singleton minimal schema.
    static MinimalSchema & getMinimalSchema();

    friend class io::FitsWriter;

     // Return a writer object that knows how to save in FITS format.  See also FitsWriter.
    virtual PTR(io::FitsWriter) makeFitsWriter(io::FitsWriter::Fits * fits) const;

    boost::array< KeyTuple<Flux>, N_FLUX_SLOTS > _slotFlux; // aliases for flux measurements
    KeyTuple<Centroid> _slotCentroid;  // alias for a centroid measurement
    KeyTuple<Shape> _slotShape;  // alias for a shape measurement
};

template <typename RecordT>
class SourceColumnViewT : public ColumnViewT<RecordT> {
public:

    typedef RecordT Record;
    typedef typename RecordT::Table Table;

    ndarray::Array<double const,1> getPsfFlux() const {
        return this->operator[](this->getTable()->getPsfFluxKey());
    }
    ndarray::Array<double const,1> getApFlux() const {
        return this->operator[](this->getTable()->getApFluxKey());
    }
    ndarray::Array<double const,1> getModelFlux() const {
        return this->operator[](this->getTable()->getModelFluxKey());
    }
    ndarray::Array<double const,1> getInstFlux() const {
        return this->operator[](this->getTable()->getInstFluxKey());
    }

    ndarray::Array<double const,1> getX() const {
        return this->operator[](this->getTable()->getCentroidKey().getX());
    }
    ndarray::Array<double const,1> getY() const {
        return this->operator[](this->getTable()->getCentroidKey().getY());
    }

    ndarray::Array<double const,1> getIxx() const {
        return this->operator[](this->getTable()->getShapeKey().getIxx());
    }
    ndarray::Array<double const,1> getIyy() const {
        return this->operator[](this->getTable()->getShapeKey().getIyy());
    }
    ndarray::Array<double const,1> getIxy() const {
        return this->operator[](this->getTable()->getShapeKey().getIxy());
    }

    /// @brief @copydoc BaseColumnView::make
    template <typename InputIterator>
    static SourceColumnViewT make(PTR(Table) const & table, InputIterator first, InputIterator last) {
        return SourceColumnViewT(BaseColumnView::make(table, first, last));
    }

protected:
    explicit SourceColumnViewT(BaseColumnView const & base) : ColumnViewT<RecordT>(base) {}
};

typedef SourceColumnViewT<SourceRecord> SourceColumnView;

#ifndef SWIG

typedef SimpleCatalogT<SourceRecord> SourceCatalog;
typedef SimpleCatalogT<SourceRecord const> ConstSourceCatalog;

DEFINE_FLUX_GETTERS(`Psf')
DEFINE_FLUX_GETTERS(`Model')
DEFINE_FLUX_GETTERS(`Ap')
DEFINE_FLUX_GETTERS(`Inst')
DEFINE_CENTROID_GETTERS
DEFINE_SHAPE_GETTERS

inline RecordId SourceRecord::getParent() const { return get(SourceTable::getParentKey()); }
inline void SourceRecord::setParent(RecordId id) { set(SourceTable::getParentKey(), id); }

inline double SourceRecord::getX() const { return get(getTable()->getCentroidKey().getX()); }
inline double SourceRecord::getY() const { return get(getTable()->getCentroidKey().getY()); }

inline double SourceRecord::getIxx() const { return get(getTable()->getShapeKey().getIxx()); }
inline double SourceRecord::getIyy() const { return get(getTable()->getShapeKey().getIyy()); }
inline double SourceRecord::getIxy() const { return get(getTable()->getShapeKey().getIxy()); }

#endif // !SWIG

}}} // namespace lsst::afw::table

#endif // !AFW_TABLE_Source_h_INCLUDED