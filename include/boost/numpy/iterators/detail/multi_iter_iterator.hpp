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
#include <boost/numpy/iterators/value_type_traits.hpp>
#include <boost/numpy/dstream.hpp>

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

        static
        boost::shared_ptr<boost::numpy::detail::iter>
        construct_iter(
            detail::multi_iter_iterator_type & iter_base
          , boost::numpy::detail::iter_flags_t special_iter_flags
          , BOOST_PP_ENUM_PARAMS(N, ndarray & arr))
        {
            // TODO: Right now, all the operand are supposed to have the same
            //       dimensionality and shape. We should use the dstream library
            //       here with a scalar core shape for all operands in order to
            //       get an automatic broadcasting of all the operand arrays.
            //
            //       Maybe this function could be out-sourced for usage also in
            //       other multi iterators. Usually it's just the iter flags
            //       that change.
            type_t & thisiter = *static_cast<type_t *>(&iter_base);

            // Define a scalar core shape for all the iterator operands.
            typedef dstream::mapping::detail::core_shape<0>::shape<>
                    scalar_core_shape_t;
            #define BOOST_NUMPY_DEF(z, n, data) \
                typedef dstream::array_definition<scalar_core_shape_t, typename BOOST_PP_CAT(ValueTypeTraits,n)::value_type>\
                        BOOST_PP_CAT(arr_def,n);
            BOOST_PP_REPEAT(N, BOOST_NUMPY_DEF, ~)
            #undef BOOST_NUMPY_DEF

            // Define the type of the loop service.
            typedef dstream::detail::loop_service_arity<N>::loop_service<BOOST_PP_ENUM_PARAMS(N, arr_def)>
                    loop_service_t;
            // Create input_array_service object for each array.
            #define BOOST_NUMPY_IN_ARR_SERVICE(z, n, data) \
                dstream::detail::input_array_service<BOOST_PP_CAT(arr_def,n)> BOOST_PP_CAT(in_arr_service,n)(BOOST_PP_CAT(arr,n));
            BOOST_PP_REPEAT(N, BOOST_NUMPY_IN_ARR_SERVICE, ~)
            #undef BOOST_NUMPY_IN_ARR_SERVICE

            // Create the loop service object.
            loop_service_t loop_service(BOOST_PP_ENUM_PARAMS(N, in_arr_service));

            // Define the iterator operand flags for the input arrays.
            #define BOOST_NUMPY_ITER_OPERAND_FLAGS(z, n, data) \
                boost::numpy::detail::iter_operand_flags_t BOOST_PP_CAT(in_arr_op_flags,n) = boost::numpy::detail::iter_operand::flags::NONE::value;\
                BOOST_PP_CAT(in_arr_op_flags,n) |= thisiter.BOOST_PP_CAT(arr_access_flags_,n) & boost::numpy::detail::iter_operand::flags::READONLY::value;\
                BOOST_PP_CAT(in_arr_op_flags,n) |= thisiter.BOOST_PP_CAT(arr_access_flags_,n) & boost::numpy::detail::iter_operand::flags::WRITEONLY::value;\
                BOOST_PP_CAT(in_arr_op_flags,n) |= thisiter.BOOST_PP_CAT(arr_access_flags_,n) & boost::numpy::detail::iter_operand::flags::READWRITE::value;
            BOOST_PP_REPEAT(N, BOOST_NUMPY_ITER_OPERAND_FLAGS, ~)
            #undef BOOST_NUMPY_ITER_OPERAND_FLAGS

            // Create the iterator operand objects.
            #define BOOST_NUMPY_ITER_OPERAND(z, n, data) \
                boost::numpy::detail::iter_operand BOOST_PP_CAT(in_arr_iter_op,n)( BOOST_PP_CAT(in_arr_service,n).get_arr(), BOOST_PP_CAT(in_arr_op_flags,n), BOOST_PP_CAT(in_arr_service,n).get_arr_bcr_data() );
            BOOST_PP_REPEAT(N, BOOST_NUMPY_ITER_OPERAND, ~)
            #undef BOOST_NUMPY_ITER_OPERAND

            // Define the iterator flags.
            boost::numpy::detail::iter_flags_t iter_flags =
                boost::numpy::detail::iter::flags::REFS_OK::value
              | boost::numpy::detail::iter::flags::DONT_NEGATE_STRIDES::value;
            iter_flags |= special_iter_flags;

            // Create the multi iterator object.
            boost::shared_ptr<boost::numpy::detail::iter> it(new boost::numpy::detail::iter(
                iter_flags
              , KEEPORDER
              , NO_CASTING
              , loop_service.get_loop_nd()
              , loop_service.get_loop_shape_data()
              , 0 // Use default buffersize.
              , BOOST_PP_ENUM_PARAMS(N, in_arr_iter_op)
            ));
            it->init_full_iteration();
            return it;
        }

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
        )
          : BOOST_PP_ENUM(N, BOOST_NUMPY_DEF_value_ptr_init, ~)
          , is_end_point_(false)
          , BOOST_PP_ENUM(N, BOOST_NUMPY_DEF_arr_access_flags_init, ~)
        #undef BOOST_NUMPY_DEF_arr_access_flags_init
        #undef BOOST_NUMPY_DEF_value_ptr_init
        {
            iter_ptr_ = Derived::construct_iter(*this, BOOST_PP_ENUM_PARAMS(N, arr));
            #define BOOST_NUMPY_DEF(z, n, data) \
                ndarray BOOST_PP_CAT(op_arr,n) = iter_ptr_->get_operand(n); \
                BOOST_PP_CAT(vtt_,n) = BOOST_PP_CAT(ValueTypeTraits,n)(BOOST_PP_CAT(op_arr,n));
            BOOST_PP_REPEAT(N, BOOST_NUMPY_DEF, ~)
            #undef BOOST_NUMPY_DEF
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
            #define BOOST_NUMPY_DEF(z, n, data)                                \
                BOOST_PP_CAT(ValueTypeTraits,n)::dereference(                  \
                    BOOST_PP_CAT(vtt_,n)                                       \
                  , BOOST_PP_CAT(value_ptr_,n)                                 \
                  , iter_ptr_->data_ptr_array_ptr_[n]                          \
                );
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

        // Define object vtt_# of the ValueTypeTraits# class
        // (using the default constructor).
        #define BOOST_NUMPY_DEF(z, n, data) \
            BOOST_PP_CAT(ValueTypeTraits,n) BOOST_PP_CAT(vtt_,n);
        BOOST_PP_REPEAT(N, BOOST_NUMPY_DEF, ~)
        #undef BOOST_NUMPY_DEF
    };
};

#undef N

#endif // BOOST_PP_ITERATION_FLAGS() == 1

#endif // BOOST_PP_IS_ITERATING
