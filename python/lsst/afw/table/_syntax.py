"""
Special Python syntactic sugar for Catalogs and Records.

This module is imported by tableLib.py, and should not need to be imported by any other module.
I've moved the code out of the .i file here to avoid recompiling when only pure-Python code is
changed.
"""

import fnmatch
import re
import numpy
import collections

def KeyBaseCov_subfields(self):
    """a tuple of Key subfield extraction indices (the lower-triangular elements)."""
    r = []
    for i in range(self.getSize()):
        for j in range(i+1):
            r.append((i,j))
    return tuple(r)

def KeyBaseCov_subkeys(self):
    """a tuple of subelement Keys (the lower-triangular elements)."""
    r = []
    for i in range(self.getSize()):
        for j in range(i+1):
            r.append(self[i,j])
    return tuple(r)

def Schema_extract(self, *patterns, **kwds):
    """
    Extract a dictionary of {<name>: <schema-item>} in which the field names
    match the given shell-style glob pattern(s).

    Any number of glob patterns may be passed; the result will be the union of all
    the result of each glob considered separately.

    Additional optional arguments may be passed as keywords:

      regex ------ A regular expression to be used in addition to any glob patterns passed
                   as positional arguments.  Note that this will be compared with re.match,
                   not re.search.

      sub -------- A replacement string (see re.MatchObject.expand) used to set the
                   dictionary keys of any fields matched by regex.  The field name in the
                   SchemaItem is not modified.

      ordered----- If True, a collections.OrderedDict will be returned instead of a standard
                   dict, with the order corresponding to the definition order of the Schema.

    """
    if kwds.pop("ordered", False):
        d = collections.OrderedDict()
    else:
        d = dict()
    regex = kwds.pop("regex", None)
    sub = kwds.pop("sub", None)
    if sub is not None and regex is None:
        raise ValueError("'sub' keyword argument to extract is invalid without 'regex' argument")
    if kwds:
        raise ValueError("Unrecognized keyword arguments for extract: %s" % ", ".join(kwds.keys()))
    for item in self:
        name = item.field.getName()
        if regex is not None:
            m = re.match(regex, name)
            if m is not None:
                if sub is not None:
                    name = m.expand(sub)
                d[name] = item
                continue # continue outer loop so we don't match the same name twice
        for pattern in patterns:
            if fnmatch.fnmatchcase(item.field.getName(), pattern):
                d[name] = item
                break # break inner loop so we don't match the same name twice
    return d

def BaseRecord_extract(self, *patterns, **kwds):
    """
    Extract a dictionary of {<name>: <field-value>} in which the field names
    match the given shell-style glob pattern(s).

    Any number of glob patterns may be passed; the result will be the union of all
    the result of each glob considered separately.

    Additional optional arguments may be passed as keywords:

      items ------ The result of a call to self.schema.extract(); this will be used instead
                   of doing any new matching, and allows the pattern matching to be reused
                   to extract values from multiple records.  This keyword is incompatible
                   with any position arguments and the regex, sub, and ordered keyword
                   arguments.

      split ------ If True, fields with named subfields (e.g. points) will be split into
                   separate items in the dict; instead of {"point": lsst.afw.geom.Point2I(2,3)},
                   for instance, you'd get {"point.x": 2, "point.y": 3}.
                   Default is False.

      regex ------ A regular expression to be used in addition to any glob patterns passed
                   as positional arguments.  Note that this will be compared with re.match,
                   not re.search.

      sub -------- A replacement string (see re.MatchObject.expand) used to set the
                   dictionary keys of any fields matched by regex.

      ordered----- If True, a collections.OrderedDict will be returned instead of a standard
                   dict, with the order corresponding to the definition order of the Schema.
                   Default is False.

    """
    d = kwds.pop("items", None)
    split = kwds.pop("split", False)
    if d is None:
        d = self.schema.extract(*patterns, **kwds).copy()
    elif kwds:
        raise ValueError("Unrecognized keyword arguments for extract: %s" % ", ".join(kwds.keys()))
    for name, schemaItem in d.items():  # can't use iteritems because we might be adding/deleting elements
        key = schemaItem.key
        if split and key.HAS_NAMED_SUBFIELDS:
            for subname, subkey in zip(key.subfields, key.subkeys):
                d["%s.%s" % (name, subname)] = self.get(subkey)
            del d[name]
        else:
            d[name] = self.get(schemaItem.key)
    return d

def BaseColumnView_extract(self, *patterns, **kwds):
    """
    Extract a dictionary of {<name>: <column-array>} in which the field names
    match the given shell-style glob pattern(s).

    Any number of glob patterns may be passed; the result will be the union of all
    the result of each glob considered separately.

    Note that extract("*", copy=True) provides an easy way to transform a row-major
    ColumnView into a possibly more efficient set of contiguous NumPy arrays.

    This routines unpacks Flag columns into full boolean arrays and covariances into dense
    (i.e. non-triangular packed) arrays with dimension (N,M,M), where N is the number of
    records and M is the dimension of the covariance matrix.  Fields with named subfields
    (e.g. points) are always split into separate dictionary items, as is done in
    BaseRecord.extract(..., split=True).

    Additional optional arguments may be passed as keywords:

      items ------ The result of a call to self.schema.extract(); this will be used instead
                   of doing any new matching, and allows the pattern matching to be reused
                   to extract values from multiple records.  This keyword is incompatible
                   with any position arguments and the regex, sub, and ordered keyword
                   arguments.

      where ------ Any expression that can be passed as indices to a NumPy array, including
                   slices, boolean arrays, and index arrays, that will be used to index
                   each column array.  This is applied before arrays are copied when
                   copy is True, so if the indexing results in an implicit copy no
                   unnecessary second copy is performed.

      copy ------- If True, the returned arrays will be contiguous copies rather than strided
                   views into the catalog.  This ensures that the lifetime of the catalog is
                   not tied to the lifetime of a particular catalog, and it also may improve
                   the performance if the array is used repeatedly.
                   Default is False.

      regex ------ A regular expression to be used in addition to any glob patterns passed
                   as positional arguments.  Note that this will be compared with re.match,
                   not re.search.

      sub -------- A replacement string (see re.MatchObject.expand) used to set the
                   dictionary keys of any fields matched by regex.

      ordered----- If True, a collections.OrderedDict will be returned instead of a standard
                   dict, with the order corresponding to the definition order of the Schema.
                   Default is False.

    """
    copy = kwds.pop("copy", False)
    where = kwds.pop("where", None)
    d = kwds.pop("items", None)
    if d is None:
        d = self.schema.extract(*patterns, **kwds).copy()
    elif kwds:
        raise ValueError("Unrecognized keyword arguments for extract: %s" % ", ".join(kwds.keys()))
    def processArray(a):
        if where is not None:
            a = a[where]
        if copy:
            a = numpy.ascontiguousarray(a)
        return a
    for name, schemaItem in d.items(): # can't use iteritems because we might be adding/deleting elements
        key = schemaItem.key
        if key.HAS_NAMED_SUBFIELDS:
            for subname, subkey in zip(key.subfields, key.subkeys):
                d["%s.%s" % (name, subname)] = processArray(self.get(subkey))
            del d[name]
        elif key.getTypeString().startswith("Cov"):
            unpacked = None
            for idx, subkey in zip(key.subfields, key.subkeys):
                i, j = idx
                array = processArray(self.get(subkey))
                if unpacked is None:
                    unpacked = numpy.zeros((array.size, key.getSize(), key.getSize()), dtype=array.dtype)
                unpacked[:,i,j] = array
                if i != j:
                    unpacked[:,j,i] = array
            d[name] = unpacked
        else:
            d[name] = processArray(self.get(schemaItem.key))
    return d