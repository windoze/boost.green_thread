//
//  forward.hpp
//  Boost.GreenThread
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// Copyright (c) 2015 Chen Xu
//

#ifndef BOOST_GREEN_THREAD_DETAIL_FORWARD_HPP
#define BOOST_GREEN_THREAD_DETAIL_FORWARD_HPP

#include <boost/chrono/system_clocks.hpp>
#include <boost/asio/basic_waitable_timer.hpp>
#include <boost/asio/io_service.hpp>

namespace boost { namespace green_thread { namespace detail {
    typedef boost::chrono::steady_clock::duration duration_t;
    typedef boost::chrono::steady_clock::time_point time_point_t;
    typedef boost::asio::basic_waitable_timer<boost::chrono::steady_clock> timer_t;
    struct scheduler_object;
    struct thread_object;
    typedef std::shared_ptr<thread_object> thread_ptr_t;
}}} // End of namespace boost::green_thread::detail

namespace boost { namespace green_thread { namespace this_thread { namespace detail {
    boost::asio::io_service &get_io_service();
}}}}    // End of namespace boost::green_thread::this_thread::detail

#endif
