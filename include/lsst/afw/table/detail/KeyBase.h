// -*- lsst-c++ -*-
#ifndef AFW_TABLE_DETAIL_KeyBase_h_INCLUDED
#define AFW_TABLE_DETAIL_KeyBase_h_INCLUDED



namespace lsst { namespace afw { namespace table { 

template <typename T> class Key;

template <typename T> class Point;
template <typename T> class Shape;
template <typename T> class Array;
template <typename T> class Covariance;

namespace detail {

class Access;

template <typename T>
class KeyBase {};

template <typename U>
class KeyBase< Point<U> > {
public:
    Key<U> getX() const;
    Key<U> getY() const;
};

template <typename U>
class KeyBase< Shape<U> > {
public:
    Key<U> getIXX() const;
    Key<U> getIYY() const;
    Key<U> getIXY() const;
};

template <typename U>
class KeyBase< Array<U> > {
public:
    Key<U> operator[](int i) const;
};

template <typename U>
class KeyBase< Covariance<U> > {
public:
    Key<U> operator()(int i, int j) const;
};

template <typename U>
class KeyBase< Covariance< Point<U> > > {
public:
    Key<U> operator()(int i, int j) const;
};

template <typename U>
class KeyBase< Covariance< Shape<U> > > {
public:
    Key<U> operator()(int i, int j) const;
};

}}}} // namespace lsst::afw::table::detail

#endif // !AFW_TABLE_DETAIL_KeyBase_h_INCLUDED
