//
//  mutex.hpp
//  Boost.GreenThread
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// Copyright (c) 2015 Chen Xu
//

#ifndef BOOST_GREEN_THREAD_MUTEX_HPP
#define BOOST_GREEN_THREAD_MUTEX_HPP

#include <deque>
#include <memory>
#include <boost/green_thread/detail/config.hpp>
#include <boost/chrono/system_clocks.hpp>
#include <boost/thread/lock_types.hpp>
#include <boost/green_thread/detail/forward.hpp>
#include <boost/green_thread/detail/spinlock.hpp>

namespace boost { namespace green_thread {
    class BOOST_GREEN_THREAD_DECL mutex {
    public:
        /// constructor
        mutex()=default;
        
        /**
         * locks the mutex, blocks if the mutex is not available
         */
        void lock();

        /**
         * tries to lock the mutex, returns if the mutex is not available
         */
        bool try_lock();
        
        /**
         * unlocks the mutex
         */
        void unlock();
    private:
        /// non-copyable
        mutex(const mutex&) = delete;
        void operator=(const mutex&) = delete;
        detail::spinlock mtx_;
        detail::thread_ptr_t owner_;
        std::deque<detail::thread_ptr_t> suspended_;
        friend struct condition_variable;
    };
    
    class BOOST_GREEN_THREAD_DECL timed_mutex {
    public:
        /// constructor
        timed_mutex()=default;

        /**
         * locks the mutex, blocks if the mutex is not available
         */
        void lock();

        /**
         * tries to lock the mutex, returns if the mutex is not available
         */
        bool try_lock();

        /**
         * tries to lock the mutex, returns if the mutex has been
         * unavailable for the specified timeout duration
         */
        template<class Rep, class Period>
        bool try_lock_for(const boost::chrono::duration<Rep,Period>& timeout_duration ) {
            return try_lock_rel(boost::chrono::duration_cast<detail::duration_t>(timeout_duration));
        }
        
        /**
         * tries to lock the mutex, returns if the mutex has been
         * unavailable until specified time point has been reached
         */
        template< class Clock, class Duration >
        bool try_lock_until(const boost::chrono::time_point<Clock,Duration>& timeout_time ) {
            return try_lock_for(timeout_time-Clock::now());
        }
        
        /**
         * unlocks the mutex
         */
        void unlock();
    private:
        /// non-copyable
        timed_mutex(const timed_mutex&) = delete;
        void operator=(const timed_mutex&) = delete;
        bool try_lock_rel(detail::duration_t d);
        void timeout_handler(detail::thread_ptr_t this_thread,
                             boost::system::error_code ec);
        
        detail::spinlock mtx_;
        detail::thread_ptr_t owner_;
        struct suspended_item {
            detail::thread_ptr_t f_;
            detail::timer_t *t_;
            bool operator==(detail::thread_ptr_t f) const { return f_==f; }
        };
        std::deque<suspended_item> suspended_;
    };
    
    class BOOST_GREEN_THREAD_DECL recursive_mutex {
    public:
        /// constructor
        recursive_mutex()=default;

        /**
         * locks the mutex, blocks if the mutex is not available
         */
        void lock();

        /**
         * tries to lock the mutex, returns if the mutex is not available
         */
        bool try_lock();

        /**
         * unlocks the mutex
         */
        void unlock();
        
    private:
        /// non-copyable
        recursive_mutex(const recursive_mutex&) = delete;
        void operator=(const recursive_mutex&) = delete;

        detail::spinlock mtx_;
        size_t level_=0;
        detail::thread_ptr_t owner_;
        std::deque<detail::thread_ptr_t> suspended_;
    };
    
    class BOOST_GREEN_THREAD_DECL recursive_timed_mutex {
    public:
        /// constructor
        recursive_timed_mutex()=default;

        /**
         * locks the mutex, blocks if the mutex is not available
         */
        void lock();
        
        /**
         * tries to lock the mutex, returns if the mutex is not available
         */
        bool try_lock();
        
        /**
         * unlocks the mutex
         */
        void unlock();
        
        /**
         * tries to lock the mutex, returns if the mutex has been
         * unavailable for the specified timeout duration
         */
        template<class Rep, class Period>
        bool try_lock_for(const boost::chrono::duration<Rep,Period>& timeout_duration) {
            return try_lock_rel(boost::chrono::duration_cast<detail::duration_t>(timeout_duration));
        }
        
        /**
         * tries to lock the mutex, returns if the mutex has been
         * unavailable until specified time point has been reached
         */
        template<class Clock, class Duration>
        bool try_lock_until(const boost::chrono::time_point<Clock,Duration>& timeout_time) {
            return try_lock_for(timeout_time-Clock::now());
        }
        
    private:
        /// non-copyable
        recursive_timed_mutex(const recursive_timed_mutex&) = delete;
        void operator=(const recursive_timed_mutex&) = delete;
        bool try_lock_rel(detail::duration_t d);
        void timeout_handler(detail::thread_ptr_t this_thread, boost::system::error_code ec);
        detail::spinlock mtx_;
        size_t level_;
        detail::thread_ptr_t owner_;
        struct suspended_item {
            detail::thread_ptr_t f_;
            detail::timer_t *t_;
            bool operator==(detail::thread_ptr_t f) const { return f_==f; }
        };
        std::deque<suspended_item> suspended_;
    };
}}  // End of namespace boost::green_thread

#endif
