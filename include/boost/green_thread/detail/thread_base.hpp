//
//  thread_base.hpp
//  Boost.GreenThread
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// Copyright (c) 2015 Chen Xu

#ifndef BOOST_GREEN_THREAD_DETAIL_THREAD_BASE_HPP
#define BOOST_GREEN_THREAD_DETAIL_THREAD_BASE_HPP

#include <memory>
#include <boost/asio/io_service.hpp>
#include <boost/asio/strand.hpp>

namespace boost { namespace green_thread { namespace detail {
    /// struct thread_base
    /**
     * can be used to comminicate with foreign threads
     */
    struct scheduler_object;
    struct thread_base {
        typedef std::shared_ptr<thread_base> ptr_t;
        
        /// destructor
        virtual ~thread_base(){};
        
        /// Pause the thread
        /**
         * NOTE: Must be called in a thread, also you need to make sure the thread will be resumed later
         */
        virtual void pause()=0;

        /// Resume the thread
        /**
         * NOTE: Must be called in the context not in this thread, the function will return immediately
         */
        virtual void resume()=0;
        
        /// Activate the thread
        /**
         * NOTE: Must be called in the thread that runs the io_service loop, but not in any thread,
         * i.e. in an asynchronous completion handle
         */
        virtual void activate()=0;
        
        /**
         * Returns the strand object associated with the thread
         */
        virtual boost::asio::strand &get_thread_strand()=0;

        /**
         * Returns the io_service object associated with the thread
         */
        inline boost::asio::io_service &get_io_service() {
            return get_thread_strand().get_io_service();
        }
        
        /**
         * Returns the scheduler the thread was created
         */
        virtual std::shared_ptr<scheduler_object> get_scheduler()=0;
    };
    
    /**
     * Returns a shared pointer to currently running thread
     */
    thread_base::ptr_t get_current_thread_ptr();
}}} // End of namespace boost::green_thread::detail

#endif
