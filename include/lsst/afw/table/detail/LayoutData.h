// -*- lsst-c++ -*-
#ifndef AFW_TABLE_DETAIL_LayoutData_h_INCLUDED
#define AFW_TABLE_DETAIL_LayoutData_h_INCLUDED



#include <vector>
#include <algorithm>

#include "boost/variant.hpp"
#include "boost/mpl/transform.hpp"
#include "boost/type_traits/remove_const.hpp"
#include "boost/type_traits/remove_reference.hpp"

#include "lsst/afw/table/detail/RecordData.h"

namespace lsst { namespace afw { namespace table {

class Layout;

/**
 *  @brief A simple pair-like struct for mapping a Field (name and description) with a Key
 *         (used for actual data access).
 */
template <typename T>
struct LayoutItem {
    Key<T> key;
    Field<T> field;
};

namespace detail {

/**
 *  @brief An internals class to hide the ugliness of the Layout implementation.
 *
 *  This can't be a full pImpl class, because some of the most important functionality
 *  is in the forEach function, a templated function we can't explicitly instantiate
 *  in a source file.  But putting all the details draws a clear line between what
 *  users should at (Layout) and what they shouldn't (this).
 *
 *  Because Layout holds LayoutData by shared pointer, one LayoutData can be shared between
 *  multiple Layouts, which use copy-on-write to create a new LayoutData if the pointer they have
 *  isn't unique.
 */
class LayoutData {
private:

    struct MakeItem {
        template <typename T>
        struct apply {
            typedef LayoutItem<T> type;
        };
    };

public:

    typedef boost::mpl::transform<FieldTypes,MakeItem>::type ItemTypes;
    typedef boost::make_variant_over<ItemTypes>::type ItemVariant;
    typedef std::vector<ItemVariant> ItemContainer;

    LayoutData() : _recordSize(sizeof(RecordData)), _items() {}

private:

    template <typename F>
    struct VisitorWrapper : public boost::static_visitor<> {

        template <typename T>
        void operator()(LayoutItem<T> const & x) const { _func(x); };
    
        void operator()(ItemVariant const & v) const {
            boost::apply_visitor(*this, v);
        }

        explicit VisitorWrapper(F func) : _func(func) {}

    private:
        F _func;
    };

    friend class table::Layout;
    friend class detail::Access;

    int _recordSize;
    ItemContainer _items;
};

}}}} // namespace lsst::afw::table::detail

#endif // !AFW_TABLE_DETAIL_LayoutData_h_INCLUDED
