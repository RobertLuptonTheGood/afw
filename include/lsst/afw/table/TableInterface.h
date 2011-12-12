// -*- lsst-c++ -*-
#ifndef AFW_TABLE_TableInterface_h_INCLUDED
#define AFW_TABLE_TableInterface_h_INCLUDED

#include "boost/iterator/transform_iterator.hpp"

#include "lsst/base.h"
#include "lsst/afw/table/RecordInterface.h"
#include "lsst/afw/table/TableBase.h"
#include "lsst/afw/table/TreeIteratorBase.h"
#include "lsst/afw/table/IteratorBase.h"
#include "lsst/afw/table/detail/Access.h"

namespace lsst { namespace afw { namespace table {

/**
 *  @brief A view into a table that provides tree-based iterators.
 *
 *  @sa TableInterface::asTree().
 */
template <typename Tag>
class TreeView : private TableBase {
public:

    typedef typename Tag::Table Table;   ///< @brief Table type this is a view into.
    typedef typename Tag::Record Record; ///< @brief Record type obtained by dereferencing iterators.
    
    typedef boost::transform_iterator<detail::RecordConverter<Record>,TreeIteratorBase> Iterator;
    typedef Iterator iterator;
    typedef Iterator const_iterator;

    Iterator begin() const {
        return Iterator(this->_beginTree(_mode), detail::RecordConverter<Record>());
    }
    Iterator end() const {
        return Iterator(this->_endTree(_mode), detail::RecordConverter<Record>());
    }

    /// @brief Unlink the record pointed at by the given iterator and return an iterator to the next record.
    Iterator unlink(Iterator const & iter) const {
        if (iter.base().getMode() != _mode) {
            throw LSST_EXCEPT(
                lsst::pex::exceptions::LogicErrorException,
                "TreeView and iterator modes do not agree."
            );
        }
        return Iterator(this->_unlink(iter.base()), detail::RecordConverter<Record>());
    }

private:

    template <typename OtherTag> friend class TableInterface;

    TreeView(TableBase const & table, TreeMode mode) : TableBase(table), _mode(mode) {}

    TreeMode _mode;
};

/**
 *  @brief A facade base class that provides most of the public interface of a table.
 *
 *  TableInterface inherits from TableBase and gives a table a consistent public interface
 *  by providing thin wrappers around TableBase member functions that return adapted types.
 *
 *  Final table classes should inherit from TableInterface, templated on a tag class that
 *  typedefs both the table class and the corresponding final record class (see SimpleTable
 *  for an example).
 *
 *  TableInterface does not provide public wrappers for member functions that add new records,
 *  because final table classes may want to control what auxiliary data is required to be present
 *  in a record.
 */
template <typename Tag>
class TableInterface : public TableBase {
public:

    typedef typename Tag::Table Table;
    typedef typename Tag::Record Record;
    typedef TreeView<Tag> Tree;
    typedef boost::transform_iterator<detail::RecordConverter<Record>,IteratorBase> Iterator;
    typedef Iterator iterator;
    typedef Iterator const_iterator;

    /**
     *  @brief Return a tree view into the table with the given TreeMode.
     *
     *  TreeViews are invalidated when the link mode is set to PARENT_ID.
     */
    Tree asTree(TreeMode mode) const { return Tree(*this, mode); }

    Iterator begin() const {
        return Iterator(this->_begin(), detail::RecordConverter<Record>());
    }
    Iterator end() const {
        return Iterator(this->_end(), detail::RecordConverter<Record>());
    }

    /// @copydoc TableBase::_unlink
    Iterator unlink(Iterator const & iter) const {
        return Iterator(this->_unlink(iter.base()), detail::RecordConverter<Record>());
    }

    /// @copydoc TableBase::_find
    Iterator find(RecordId id) const {
        return Iterator(this->_find(id), detail::RecordConverter<Record>());
    }

    /// @copydoc TableBase::_get
    Record operator[](RecordId id) const {
        return detail::Access::makeRecord<Record>(this->_get(id));
    }

protected:

    TableInterface(
        Schema const & schema,
        int capacity,
        int nRecordsPerBlock,
        PTR(IdFactory) const & idFactory = PTR(IdFactory)(),
        PTR(AuxBase) const & aux = PTR(AuxBase)()
    ) : TableBase(schema, capacity, nRecordsPerBlock, idFactory, aux) {}

    Record _addRecord(RecordId id, PTR(AuxBase) const & aux = PTR(AuxBase)()) const {
        return detail::Access::makeRecord<Record>(this->TableBase::_addRecord(id, aux));
    }

    Record _addRecord(PTR(AuxBase) const & aux = PTR(AuxBase)()) const {
        return detail::Access::makeRecord<Record>(this->TableBase::_addRecord(aux));
    }
    
};

}}} // namespace lsst::afw::table

#endif // !AFW_TABLE_TableInterface_h_INCLUDED
