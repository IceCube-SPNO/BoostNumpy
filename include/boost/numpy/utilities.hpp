/**
 * $Id$
 *
 * Copyright (C)
 * 2015 - $Date$
 *     Martin Wolf <boostnumpy@martin-wolf.org>
 *
 * @file    boost/numpy/utilities.hpp
 * @version $Revision$
 * @date    $Date$
 * @author  Martin Wolf <boostnumpy@martin-wolf.org>
 *
 * @brief This file defines utility functions similar to those universal
 *        functions available in the numpy module.
 */
#ifndef BOOST_NUMPY_UTILITIES_HPP_INCLUDED
#define BOOST_NUMPY_UTILITIES_HPP_INCLUDED

#include <boost/numpy/dtype.hpp>
#include <boost/numpy/ndarray.hpp>

namespace boost {
namespace numpy {

/**
 * @brief Equivalent to the ``numpy.all`` function.
 *        Checks if all array elements along the given axis evaluate to
 *        ``true``, by calling the PyArray_All function. The returned
 *        boost::python::object object is either a numpy scalar or a numpy
 *        ndarray.
 */
python::object
all(ndarray const & a, int axis);

void
all(ndarray const & a, int axis, ndarray & out);

/**
 * @brief Equivalent to the ``numpy.any`` function.
 *        Checks if any array elements along the given axis evalutes to
 *        ``true``, by calling the PyArray_Any function. The returned
 *        boost::python::object object is either a numpy scalar or a numpy
 *        ndarray.
 */
python::object
any(ndarray const & a, int axis);

void
any(ndarray const & a, int axis, ndarray & out);

/**
 * @brief Compares element-wise the content the ndarray a0 and a1 if they are
 *        equal. The resulting ndarray is a boolean array with a shape that
 *        fits the broadcasted shape of the two i
 */
ndarray
equal(ndarray const & x1, ndarray const & x2);

void
equal(ndarray const & x1, ndarray const & x2, ndarray & out);

/**
 * @brief Creates an empty ndarray of the same shape and data type as the given
 *     ndarray.
 */
ndarray
empty_like(ndarray const & a);

/**
 * @brief Creates an empty ndarray of the same shape as the given ndarray but
 *    with the given data type.
 */
ndarray
empty_like(ndarray const & a, dtype const & dt);

}// namespace numpy
}// namespace boost

#endif // !BOOST_NUMPY_UTILITIES_HPP_INCLUDED
