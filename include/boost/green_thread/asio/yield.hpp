//
//  yield.hpp
//  Boost.GreenThread
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// Copyright (c) 2015 Chen Xu
//

#ifndef BOOST_GREEN_THREAD_ASIO_YIELD_HPP
#define BOOST_GREEN_THREAD_ASIO_YIELD_HPP

#include <memory>
#include <functional>
#include <boost/system/error_code.hpp>
#include <boost/chrono/system_clocks.hpp>

namespace boost { namespace green_thread { namespace asio {
    /**
     * class yield_t
     *
     * Instance of the class can be used as completion handler for Boost.ASIO async_* call,
     * makes calls block current thread until completion.
     */
    class yield_t {
    public:
        /// constructor
        constexpr yield_t()
        : ec_(0)
        {}

        /**
         * Return a yield context that sets the specified error_code.
         *
         * @param ec a reference of error_code to store returned errors
         */
        yield_t operator[](boost::system::error_code &ec) const {
            yield_t tmp;
            tmp.ec_ = &ec;
            return tmp;
        }
        
        //private:
        boost::system::error_code *ec_;
    };
    
    /// predefined instance of yield_t can be used directly
    constexpr yield_t yield;
}}} // End of namespace boost::green_thread::asio

#include <boost/green_thread/asio/detail/yield.hpp>

#endif
