/**
 * $Id$
 *
 * Copyright (C)
 * 2013
 *     Martin Wolf <martin.wolf@icecube.wisc.edu>
 *     and the IceCube Collaboration <http://www.icecube.wisc.edu>
 *
 * \file    boost/numpy/detail/max.hpp
 * \version $Revision$
 * \date    $Date$
 * \author  Martin Wolf <martin.wolf@icecube.wisc.edu>
 *
 * \brief This file defines the max template function for a variadic number of
 *        values using the C++03 standard.
 *
 *        This file is distributed under the Boost Software License,
 *        Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
 *        http://www.boost.org/LICENSE_1_0.txt).
 */
#ifndef BOOST_NUMPY_DETAIL_MAX_HPP_INCLUDED
#define BOOST_NUMPY_DETAIL_MAX_HPP_INCLUDED

#include <boost/preprocessor/arithmetic/add.hpp>
#include <boost/preprocessor/iteration/local.hpp>
#include <boost/preprocessor/repetition/enum_params.hpp>

#include <boost/numpy/limits.hpp>

namespace boost {
namespace numpy {
namespace detail {

template <class T>
T max(T x, T y)
{
    return x > y ? x : y;
}

#define BOOST_PP_LOCAL_LIMITS (2, BOOST_PP_ADD(BOOST_NUMPY_LIMIT_INPUT_ARITY, 1))
#define BOOST_PP_LOCAL_MACRO(n)                                                \
    template <class T>                                                         \
    T max(T x, BOOST_PP_ENUM_PARAMS(n, T x))                                   \
    {                                                                          \
        return max(x, max(BOOST_PP_ENUM_PARAMS(n, x)));                        \
    }
#include BOOST_PP_LOCAL_ITERATE()

}/*namespace detail*/
}/*namespace numpy*/
}/*namespace boost*/

#endif // ! BOOST_NUMPY_DETAIL_MAX_HPP_INCLUDED
