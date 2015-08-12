//
//  task_base.hpp
//  Boost.GreenThread
//
// Copyright 2015 Chen Xu
// Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_GREEN_THREAD_FUTURE_DETAIL_TASK_BASE_HPP
#define BOOST_GREEN_THREAD_FUTURE_DETAIL_TASK_BASE_HPP

#include <cstddef>

#include <boost/config.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/throw_exception.hpp>
#include <boost/utility.hpp>
#include <boost/green_thread/future/detail/shared_state.hpp>

namespace boost { namespace green_thread { namespace detail {
    template<typename R, typename ...Args>
    struct task_base : public shared_state<R> {
        typedef boost::intrusive_ptr<task_base>  ptr_t;
        virtual ~task_base() {}
        virtual void run(Args&&... args) = 0;
    };
}}} // End of namespace boost::green_thread::detail

#endif
