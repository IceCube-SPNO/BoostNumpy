/**
 * $Id$
 *
 * Copyright (C)
 * 2013 - $Date$
 *     Martin Wolf <boostnumpy@martin-wolf.org>
 * 2010-2012
 *     Jim Bosch
 *
 * @file    boost/numpy/ndarray.cpp
 * @version $Revision$
 * @date    $Date$
 * @author  Martin Wolf <boostnumpy@martin-wolf.org>,
 *          Jim Bosch
 *
 * @brief This file implements the boost::numpy::ndarray object manager.
 *
 *        This file is distributed under the Boost Software License,
 *        Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
 *        http://www.boost.org/LICENSE_1_0.txt).
 */
#define BOOST_NUMPY_INTERNAL_IMPL
#include <boost/numpy/internal_impl.hpp>

#include <stdint.h>

#include <vector>

#include <boost/python/refcount.hpp>
#include <boost/python/object_protocol.hpp>

#include <boost/numpy/numpy_c_api.hpp>
#include <boost/numpy/object_manager_traits_impl.hpp>
#include <boost/numpy/iterators/flat_iterator.hpp>
#include <boost/numpy/iterators/value_type_traits.hpp>
#include <boost/numpy/ndarray.hpp>

BOOST_NUMPY_OBJECT_MANAGER_TRAITS_IMPL(boost::numpy::ndarray, PyArray_Type);

namespace boost {
namespace numpy {

namespace detail {

//______________________________________________________________________________
inline
ndarray::flags
npy_array_flags_to_bn_ndarray_flags(int flags)
{
    return ndarray::flags(flags);
}

//______________________________________________________________________________
inline
int
bn_ndarray_flags_to_npy_array_flags(ndarray::flags flags)
{
    return int(flags);
}

//______________________________________________________________________________
bool
is_c_contiguous(
    std::vector<Py_intptr_t> const & shape,
    std::vector<Py_intptr_t> const & strides,
    int                              itemsize)
{
    std::vector<Py_intptr_t>::const_reverse_iterator i = shape.rbegin();
    std::vector<Py_intptr_t>::const_reverse_iterator j = strides.rbegin();
    int total = itemsize;
    for(; i != shape.rend(); ++i, ++j)
    {
        if(total != *j) {
            return false;
        }
        total *= (*i);
    }
    return true;
}

//______________________________________________________________________________
bool
is_f_contiguous(
    std::vector<Py_intptr_t> const & shape,
    std::vector<Py_intptr_t> const & strides,
    int                              itemsize)
{
    std::vector<Py_intptr_t>::const_iterator i = shape.begin();
    std::vector<Py_intptr_t>::const_iterator j = strides.begin();
    int total = itemsize;
    for(; i != shape.end(); ++i, ++j)
    {
        if(total != *j) {
            return false;
        }
        total *= (*i);
    }
    return true;
}

//______________________________________________________________________________
bool
is_aligned(
    std::vector<Py_intptr_t> const & strides,
    int                              itemsize)
{
    std::vector<Py_intptr_t>::const_iterator i = strides.begin();
    for(; i != strides.end(); ++i)
    {
        if(*i % itemsize) {
            return false;
        }
    }
    return true;
}

//______________________________________________________________________________
inline
PyArray_Descr *
incref_dtype(dtype const & dt)
{
    Py_INCREF(dt.ptr());
    return reinterpret_cast<PyArray_Descr*>(dt.ptr());
}

//______________________________________________________________________________
ndarray
from_data_impl(
    void *                           data
  , dtype const &                    dt
  , std::vector<Py_intptr_t> const & shape
  , std::vector<Py_intptr_t> const & strides
  , python::object const *           owner
  , bool                             writeable
  , bool                             set_owndata_flag
)
{
    if(shape.size() != strides.size())
    {
        PyErr_SetString(PyExc_ValueError,
            "Length of shape and strides arrays do not match.");
        python::throw_error_already_set();
    }
    int itemsize = dt.get_itemsize();

    // Calculate the array flags.
    ndarray::flags flags = ndarray::NONE;
    if(writeable)
        flags = flags | ndarray::WRITEABLE;
    if(is_c_contiguous(shape, strides, itemsize))
        flags = flags | ndarray::C_CONTIGUOUS;
    if(is_f_contiguous(shape, strides, itemsize))
        flags = flags | ndarray::F_CONTIGUOUS;
    if(is_aligned(strides, itemsize))
        flags = flags | ndarray::ALIGNED;
    if( set_owndata_flag && ( (!owner) || (owner && (*owner) == python::object()) ) )
        flags = flags | ndarray::OWNDATA;

    ndarray arr(python::detail::new_reference(
        PyArray_NewFromDescr(
            &PyArray_Type,
            incref_dtype(dt),
            int(shape.size()),
            const_cast<Py_intptr_t*>(&shape.front()),
            const_cast<Py_intptr_t*>(&strides.front()),
            data,
            bn_ndarray_flags_to_npy_array_flags(flags),
            NULL)));
    if(owner && (*owner) != python::object()) {
        arr.set_base(*owner);
    }
    return arr;
}

//______________________________________________________________________________
ndarray
from_data_impl(
    void *                 data
  , dtype const &          dt
  , python::object const & shape
  , python::object const & strides
  , python::object const * owner
  , bool                   writeable
  , bool                   set_owndata_flag
)
{
    std::vector<Py_intptr_t> shape_(python::len(shape));
    std::vector<Py_intptr_t> strides_(python::len(strides));
    if(shape_.size() != strides_.size())
    {
        PyErr_SetString(PyExc_ValueError,
            "Length of shape and strides arrays do not match.");
        python::throw_error_already_set();
    }
    for(std::size_t i=0; i<shape_.size(); ++i)
    {
        shape_[i]   = python::extract<Py_intptr_t>(shape[i]);
        strides_[i] = python::extract<Py_intptr_t>(strides[i]);
    }
    return from_data_impl(data, dt, shape_, strides_, owner, writeable, set_owndata_flag);
}

}/*namespace detail*/

//______________________________________________________________________________
ndarray
ndarray::
view(dtype const & dt) const
{
    return ndarray(python::detail::new_reference(
        PyObject_CallMethod(this->ptr(), const_cast<char*>("view"), const_cast<char*>("O"), dt.ptr())));
}

//______________________________________________________________________________
ndarray
ndarray::
flatten(std::string const & order) const
{
    return ndarray(python::detail::new_reference(
        PyObject_CallMethod(this->ptr(), const_cast<char*>("flatten"), const_cast<char*>("(s)"), const_cast<char*>(order.c_str()))));
}

//______________________________________________________________________________
ndarray
ndarray::
copy(std::string const & order) const
{
    return ndarray(python::detail::new_reference(
        PyObject_CallMethod(this->ptr(), const_cast<char*>("copy"), const_cast<char*>("(s)"), const_cast<char*>(order.c_str()))));
}

//______________________________________________________________________________
ndarray
ndarray::
deepcopy(std::string const & order) const
{
    ndarray arr = this->copy(order);

    if(arr.is_object_array())
    {
        python::object copy_module = python::import("copy");
        python::object deepcopy_fct = copy_module.attr("deepcopy");

        // The ndarray is an object array. So we need to copy each object
        // individually.
        typedef iterators::flat_iterator< iterators::single_value<uintptr_t> >
                iter_t;
        iter_t iter(arr, detail::iter_operand::flags::READWRITE::value);
        while(! iter.is_end())
        {
            iterators::single_value<uintptr_t>::value_ref_type obj_ptr_value_ref = *iter;

            python::object obj(python::detail::borrowed_reference(reinterpret_cast<PyObject*>(obj_ptr_value_ref)));
            python::object obj_copy = deepcopy_fct(obj);
            python::xdecref<PyObject>(obj.ptr());
            python::incref<PyObject>(obj_copy.ptr());
            obj_ptr_value_ref = reinterpret_cast<uintptr_t>(obj_copy.ptr());

            ++iter;
        }
    }

    return arr;
}

//______________________________________________________________________________
bool
ndarray::
check_flags(flags const flags) const
{
    return PyArray_CHKFLAGS(reinterpret_cast<PyArrayObject*>(const_cast<PyObject*>(this->ptr())), (int)flags);
}

//______________________________________________________________________________
void
ndarray::
clear_flags(flags const flags)
{
#if NPY_FEATURE_VERSION >= 0x00000007
    PyArray_CLEARFLAGS((PyArrayObject*)this->ptr(), (int)flags);
#else
    // There is no PyArray_CLEARFLAGS function in numpy prior to version 1.7.
    // But there we still have full access to the members of the PyArrayObject
    // struct. So we will make use of that.
    reinterpret_cast<PyArrayObject*>(this->ptr())->flags &= ~(int)flags;
#endif
}

//______________________________________________________________________________
void
ndarray::
enable_flags(flags const flags)
{
#if NPY_FEATURE_VERSION >= 0x00000007
    PyArray_ENABLEFLAGS((PyArrayObject*)this->ptr(), (int)flags);
#else
    // There is no PyArray_ENABLEFLAGS function in numpy prior to version 1.7.
    // But there we still have full access to the members of the PyArrayObject
    // struct. So we will make use of that.
    reinterpret_cast<PyArrayObject*>(this->ptr())->flags |= (int)flags;
#endif
}

//______________________________________________________________________________
dtype
ndarray::
get_dtype() const
{
    return dtype(python::detail::borrowed_reference(PyArray_DESCR((PyArrayObject*)this->ptr())));
}

//______________________________________________________________________________
python::object
ndarray::
get_base() const
{
    PyObject* base = PyArray_BASE((PyArrayObject*)this->ptr());
    if(base == NULL)
        return python::object();
    return python::object(python::detail::borrowed_reference(base));
}

//______________________________________________________________________________
void
ndarray::
set_base(python::object const & base)
{
    boost::python::xincref(base.ptr());

#if NPY_FEATURE_VERSION >= 0x00000007
    if(PyArray_SetBaseObject((PyArrayObject*)this->ptr(), base.ptr()))
    {
        PyErr_SetString(PyExc_RuntimeError, "Could not set base of PyArrayObject!");
        boost::python::throw_error_already_set();
    }
#else
    PyArray_BASE((PyArrayObject*)this->ptr()) = base.ptr();
#endif
}

//______________________________________________________________________________
ndarray::flags const
ndarray::
get_flags() const
{
    return detail::npy_array_flags_to_bn_ndarray_flags(PyArray_FLAGS((PyArrayObject*)this->ptr()));
}

//______________________________________________________________________________
int
ndarray::
get_nd() const
{
    return PyArray_NDIM((PyArrayObject*)this->ptr());
}

//______________________________________________________________________________
intptr_t
ndarray::
get_size() const
{
    return PyArray_Size(this->ptr());
}

//______________________________________________________________________________
intptr_t const *
ndarray::
get_shape() const
{
    return PyArray_DIMS((PyArrayObject*)this->ptr());
}

//______________________________________________________________________________
intptr_t const *
ndarray::
get_strides() const
{
    return PyArray_STRIDES((PyArrayObject*)this->ptr());
}

//______________________________________________________________________________
char *
ndarray::
get_data() const
{
    return PyArray_BYTES((PyArrayObject*)this->ptr());
}

//______________________________________________________________________________
bool
ndarray::
is_object_array() const
{
    return dtype::equivalent(get_dtype(), dtype::get_builtin<boost::python::object>());
}

//______________________________________________________________________________
ndarray
ndarray::
transpose() const
{
    return ndarray(python::detail::new_reference(
        PyArray_Transpose(reinterpret_cast<PyArrayObject*>(this->ptr()), NULL)));
}

//______________________________________________________________________________
ndarray
ndarray::
squeeze() const
{
    return ndarray(python::detail::new_reference(
        PyArray_Squeeze(reinterpret_cast<PyArrayObject*>(this->ptr()))));
}

//______________________________________________________________________________
ndarray
ndarray::
reshape(python::tuple const & shape) const
{
    return ndarray(python::detail::new_reference(
        PyArray_Reshape(reinterpret_cast<PyArrayObject*>(this->ptr()), shape.ptr())));
}

//______________________________________________________________________________
ndarray
ndarray::
reshape(python::list const & shape) const
{
    return ndarray(python::detail::new_reference(
        PyArray_Reshape(reinterpret_cast<PyArrayObject*>(this->ptr()), shape.ptr())));
}

//______________________________________________________________________________
ndarray
ndarray::
reshape(std::vector<intptr_t> const & shape) const
{
    python::list shape_list;
    for(size_t i=0; i<shape.size(); ++i)
    {
        shape_list.append(shape[i]);
    }
    return reshape(shape_list);
}

//______________________________________________________________________________
python::object
ndarray::
scalarize() const
{
    PyArrayObject* arr = reinterpret_cast<PyArrayObject*>(ptr());

    // Check if we got a 1x1 ndarray.
    if(this->get_nd() == 1 && this->shape(0) == 1)
    {
        return python::object(python::detail::new_reference(
            PyArray_ToScalar(PyArray_DATA(arr), arr)));
    }

    // This ndarray is either a 0-dimensional array or something else than 1x1.
    // The reference count is decremented by PyArray_Return so we need to
    // increment it first.
    Py_INCREF(ptr());
    return python::object(python::detail::new_reference(
        PyArray_Return(arr)));
}

//______________________________________________________________________________
bool
ndarray::
has_shape(std::vector<intptr_t> const & shape) const
{
    int const nd = this->get_nd();
    if(nd != int(shape.size())) {
        return false;
    }
    intptr_t const * this_shape = this->get_shape();
    for(int i=0; i<nd; ++i)
    {
        if(this_shape[i] != shape[i]) {
            return false;
        }
    }
    return true;
}

//______________________________________________________________________________
python::object
ndarray::
get_bpo_item(python::object const & obj) const
{
    python::object item = python::api::getitem((python::object)*this, obj);
    return item;
}

//______________________________________________________________________________
ndarray
zeros(
    int              nd,
    intptr_t const * shape,
    dtype const &    dt)
{
    return ndarray(python::detail::new_reference(
        PyArray_Zeros(nd, const_cast<intptr_t*>(shape), detail::incref_dtype(dt), 0)));
}

//______________________________________________________________________________
ndarray
zeros(
    python::tuple const & shape,
    dtype const &         dt)
{
    int nd = python::len(shape);
    intptr_t dims[nd];
    for(int n=0; n<nd; ++n) {
        dims[n] = python::extract<intptr_t>(shape[n]);
    }
    return zeros(nd, dims, dt);
}

//______________________________________________________________________________
ndarray
zeros(std::vector<intptr_t> const & shape, dtype const & dt)
{
    return zeros(int(shape.size()), &(shape.front()), dt);
}

//______________________________________________________________________________
ndarray
empty(
    int              nd,
    intptr_t const * shape,
    dtype const &    dt)
{
    return ndarray(python::detail::new_reference(
        PyArray_Empty(nd, const_cast<intptr_t*>(shape), detail::incref_dtype(dt), 0)));
}

//______________________________________________________________________________
ndarray
empty(
    python::tuple const & shape,
    dtype const &         dt)
{
    int nd = python::len(shape);
    intptr_t dims[nd];
    for(int n=0; n<nd; ++n) {
        dims[n] = python::extract<intptr_t>(shape[n]);
    }
    return empty(nd, dims, dt);
}

//______________________________________________________________________________
ndarray
empty(std::vector<intptr_t> const & shape, dtype const & dt)
{
    return empty(int(shape.size()), &(shape.front()), dt);
}

//______________________________________________________________________________
ndarray
array(python::object const & obj)
{
    // We need to set the ndarray::ENSUREARRAY flag here, because the array
    // function is supposed to construct a numpy.ndarray object and not
    // something else.
    return ndarray(python::detail::new_reference(
        PyArray_FromAny(obj.ptr(), NULL, 0, 0, ndarray::ENSUREARRAY, NULL)));
}

//______________________________________________________________________________
ndarray
array(python::object const & obj, dtype const & dt)
{
    // We need to set the ndarray::ENSUREARRAY flag here, because the array
    // function is supposed to construct a numpy.ndarray object and not
    // something else.
    return ndarray(python::detail::new_reference(
        PyArray_FromAny(obj.ptr(), detail::incref_dtype(dt), 0, 0, ndarray::ENSUREARRAY, NULL)));
}

//______________________________________________________________________________
ndarray
from_object(
    python::object const & obj,
    dtype const &          dt,
    int                    nd_min,
    int                    nd_max,
    ndarray::flags         flags)
{
    int requirements = detail::bn_ndarray_flags_to_npy_array_flags(flags);
    return ndarray(python::detail::new_reference(
        PyArray_FromAny(
            obj.ptr(),
            detail::incref_dtype(dt),
            nd_min,
            nd_max,
            requirements,
            NULL)));
}

//______________________________________________________________________________
ndarray
from_object(
    python::object const & obj,
    int                    nd_min,
    int                    nd_max,
    ndarray::flags         flags)
{
    int requirements = detail::bn_ndarray_flags_to_npy_array_flags(flags);
    return ndarray(python::detail::new_reference(
        PyArray_FromAny(
            obj.ptr(),
            NULL,
            nd_min,
            nd_max,
            requirements,
            NULL)));
}

//______________________________________________________________________________
bool
is_any_scalar(python::object const & obj)
{
    return PyArray_IsAnyScalar(obj.ptr());
}

//______________________________________________________________________________
bool
is_array_scalar(python::object const & obj)
{
    return PyArray_CheckScalar(obj.ptr());
}

//______________________________________________________________________________
bool
is_ndarray(python::object const & obj)
{
    return PyArray_Check(obj.ptr());
}

//______________________________________________________________________________
bool
copy_into(ndarray & dst, ndarray const & src)
{
    return (! PyArray_CopyInto(
                    reinterpret_cast<PyArrayObject*>(dst.ptr())
                  , reinterpret_cast<PyArrayObject*>(src.ptr())));
}

}// namespace numpy
}// namespace boost
