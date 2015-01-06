/**
 * $Id$
 *
 * Copyright (C)
 * 2014 - $Date$
 *     Martin Wolf <boostnumpy@martin-wolf.org>
 *
 * @file    boost/numpy/iterators/detail/multi_iter_iterator.hpp
 * @version $Revision$
 * @date    $Date$
 * @author  Martin Wolf <boostnumpy@martin-wolf.org>
 *
 * @brief This file defines the
 *        boost::numpy::iterators::detail::multi_iter_iterator
 *        template providing the base for all BoostNumpy C++ style iterators
 *        using the boost::numpy::detail::iter class iterating over multiple
 *        ndarrays at once.
 *        Due to the multiple operands, the dereference method
 *        always just returns a boolean value, but the individual values can
 *        be accessed through the value_ptr0, value_ptr1, ... member variables
 *        being pointers to the individual data values.
 *        The value type of each operand is specified via a value type traits
 *        class, which also provides the appropriate dereferencing procedure.
 *
 *        This file is distributed under the Boost Software License,
 *        Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
 *        http://www.boost.org/LICENSE_1_0.txt).
 */
#if !defined(BOOST_PP_IS_ITERATING)

#ifndef BOOST_NUMPY_ITERATORS_DETAIL_MULTI_ITER_ITERATOR_HPP_INCLUDED
#define BOOST_NUMPY_ITERATORS_DETAIL_MULTI_ITER_ITERATOR_HPP_INCLUDED 1

#include <boost/preprocessor/iterate.hpp>
#include <boost/preprocessor/repetition/enum.hpp>

#include <boost/iterator/iterator_facade.hpp>
#include <boost/python.hpp>

#include <boost/numpy/limits.hpp>
#include <boost/numpy/ndarray.hpp>
#include <boost/numpy/detail/iter.hpp>

namespace boost {
namespace numpy {
namespace iterators {
namespace detail {

struct multi_iter_iterator_type
{};

template <int n>
struct multi_iter_iterator
{};

#define BOOST_PP_ITERATION_PARAMS_1 \
    (4, (2, BOOST_NUMPY_LIMIT_INPUT_AND_OUTPUT_ARITY, <boost/numpy/iterators/detail/multi_iter_iterator.hpp>, 1))
#include BOOST_PP_ITERATE()

}// namespace detail
}// namespace iterators
}// namespace numpy
}// namespace boost

#endif // ! BOOST_NUMPY_ITERATORS_DETAIL_MULTI_ITER_ITERATOR_HPP_INCLUDED
#else

#if BOOST_PP_ITERATION_FLAGS() == 1

#define N BOOST_PP_ITERATION()

template <>
struct multi_iter_iterator<N>
{
    template <class Derived, class CategoryOrTraversal, BOOST_PP_ENUM_PARAMS(N, typename ValueTypeTraits)>
    class impl
      : public boost::iterator_facade<
          Derived
        , bool // ValueType
        , CategoryOrTraversal
        , bool // ValueRefType
        //, DifferenceType
        >
      , public multi_iter_iterator_type
    {
      public:
        typedef multi_iter_iterator<N>::impl<Derived, CategoryOrTraversal, BOOST_PP_ENUM_PARAMS(N, ValueTypeTraits)>
                type_t;
        typedef typename boost::iterator_facade<Derived, bool, CategoryOrTraversal, bool>::difference_type
                difference_type;

        // Default constructor.
        #define BOOST_NUMPY_DEF_value_ptr_init(z, n, data) \
            BOOST_PP_CAT(value_ptr_,n)(NULL)
        #define BOOST_NUMPY_DEF_arr_access_flags_init(z, n, data) \
            BOOST_PP_CAT(arr_access_flags_,n)(boost::numpy::detail::iter_operand::flags::READONLY::value)
        impl()
          : BOOST_PP_ENUM(N, BOOST_NUMPY_DEF_value_ptr_init, ~)
          , is_end_point_(true)
          , BOOST_PP_ENUM(N, BOOST_NUMPY_DEF_arr_access_flags_init, ~)
        #undef BOOST_NUMPY_DEF_arr_access_flags_init
        #undef BOOST_NUMPY_DEF_value_ptr_init
        {}

        // Explicit constructor.
        #define BOOST_NUMPY_DEF_value_ptr_init(z, n, data) \
            BOOST_PP_CAT(value_ptr_,n)(NULL)
        #define BOOST_NUMPY_DEF_arr_access_flags_init(z, n, data) \
            BOOST_PP_CAT(arr_access_flags_,n)(BOOST_PP_CAT(arr_access_flags,n))
        explicit impl(
            BOOST_PP_ENUM_PARAMS(N, ndarray & arr)
          , BOOST_PP_ENUM_PARAMS(N, boost::numpy::detail::iter_operand_flags_t arr_access_flags)
          //, iter_construct_fct_ptr_t iter_construct_fct
        )
          : BOOST_PP_ENUM(N, BOOST_NUMPY_DEF_value_ptr_init, ~)
          , is_end_point_(false)
          , BOOST_PP_ENUM(N, BOOST_NUMPY_DEF_arr_access_flags_init, ~)
        #undef BOOST_NUMPY_DEF_arr_access_flags_init
        #undef BOOST_NUMPY_DEF_value_ptr_init
        {
            iter_ptr_ = Derived::construct_iter(*this, BOOST_PP_ENUM_PARAMS(N, arr));
        }

        // Copy constructor.
        #define BOOST_NUMPY_DEF_value_ptr_init(z, n, data) \
            BOOST_PP_CAT(value_ptr_,n)(NULL)
        #define BOOST_NUMPY_DEF_arr_access_flags_copy(z, n, data) \
            BOOST_PP_CAT(arr_access_flags_,n)(other.BOOST_PP_CAT(arr_access_flags_,n))
        impl(type_t const & other)
          : BOOST_PP_ENUM(N, BOOST_NUMPY_DEF_value_ptr_init, ~)
          , is_end_point_(other.is_end_point_)
          , BOOST_PP_ENUM(N, BOOST_NUMPY_DEF_arr_access_flags_copy, ~)
        #undef BOOST_NUMPY_DEF_arr_access_flags_copy
        #undef BOOST_NUMPY_DEF_value_ptr_init
        {
            if(other.iter_ptr_.get()) {
                iter_ptr_ = boost::shared_ptr<boost::numpy::detail::iter>(new boost::numpy::detail::iter(*other.iter_ptr_));
            }
        }

        // Creates an interator that points to the first element.
        Derived begin() const
        {
            Derived it(*static_cast<Derived*>(const_cast<type_t*>(this)));
            it.reset();
            return it;
        }

        // Creates an iterator that points to the element after the last element.
        Derived end() const
        {
            Derived it(*static_cast<Derived*>(const_cast<type_t*>(this)));
            it.is_end_point_ = true;
            return it;
        }

        void
        increment()
        {
            if(is_end())
            {
                reset();
            }
            else if(! iter_ptr_->next())
            {
                // We reached the end of the iteration. So we need to put this
                // iterator into the END state, wich is (by definition) indicated
                // through the is_end_point_ member variable set to ``true``.
                // Note: We still keep the iterator object, in case the user wants
                //       to reset the iterator and start iterating from the
                //       beginning.
                is_end_point_ = true;
            }
        }

        bool
        equal(type_t const & other) const
        {
            //std::cout << "iter_iterator: equal" << std::endl;
            if(is_end_point_ && other.is_end_point_)
            {
                return true;
            }
            // Check if one of the two iterators is the END state.
            if(is_end_point_ || other.is_end_point_)
            {
                return false;
            }
            // If the data pointers point to the same address, we are equal.
            return (iter_ptr_->get_data(0) == other.iter_ptr_->get_data(0));
        }

        bool
        reset(bool throws=true)
        {
            is_end_point_ = false;
            return iter_ptr_->reset(throws);
        }

        bool
        is_end() const
        {
            return is_end_point_;
        }

        bool
        dereference()
        {
            #define BOOST_NUMPY_DEF(z, n, data) \
                BOOST_PP_CAT(ValueTypeTraits,n)::dereference( BOOST_PP_CAT(value_ptr_,n), iter_ptr_->data_ptr_array_ptr_[n] );
            BOOST_PP_REPEAT(N, BOOST_NUMPY_DEF, ~)
            #undef BOOST_NUMPY_DEF
            return true;
        }

        // Define the value_ptr_# pointers to the array values.
        #define BOOST_NUMPY_DEF(z, n, data) \
            typename BOOST_PP_CAT(ValueTypeTraits,n)::value_ptr_type BOOST_PP_CAT(value_ptr_,n);
        BOOST_PP_REPEAT(N, BOOST_NUMPY_DEF, ~)
        #undef BOOST_NUMPY_DEF

      protected:
        boost::shared_ptr<boost::numpy::detail::iter> iter_ptr_;
        bool is_end_point_;
        // Stores if the array is readonly, writeonly or readwrite'able.
        #define BOOST_NUMPY_DEF(z, n, data) \
            boost::numpy::detail::iter_operand_flags_t BOOST_PP_CAT(arr_access_flags_,n);
        BOOST_PP_REPEAT(N, BOOST_NUMPY_DEF, ~)
        #undef BOOST_NUMPY_DEF
    };
};

#undef N

#endif // BOOST_PP_ITERATION_FLAGS() == 1

#endif // BOOST_PP_IS_ITERATING
