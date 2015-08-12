//
//  thread_object.hpp
//  Boost.GreenThread
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// Copyright (c) 2015 Chen Xu
//

#ifndef BOOST_GREEN_THREAD_THREAD_OBJECT_HPP
#define BOOST_GREEN_THREAD_THREAD_OBJECT_HPP

#include <memory>
#include <deque>
#include <map>
#include <exception>
#include <boost/assert.hpp>
#include <boost/atomic/atomic.hpp>
#include <boost/chrono/system_clocks.hpp>
#include <boost/asio/basic_waitable_timer.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/strand.hpp>
#include <boost/system/error_code.hpp>
#include <boost/coroutine/coroutine.hpp>
#include <boost/green_thread/exceptions.hpp>
#include <boost/green_thread/detail/thread_base.hpp>
#include <boost/green_thread/detail/thread_data.hpp>
#include <boost/green_thread/detail/spinlock.hpp>

#ifdef __APPLE_CC__
// Clang on OS X doesn't support thread_local
#   define THREAD_LOCAL __thread
#else
#   define THREAD_LOCAL thread_local
#endif

#if defined(DEBUG) && !defined(NDEBUG)
#   define CHECK_CALLER(f) do { if (!f->caller_) assert(false); } while(0)
#else
#   define CHECK_CALLER(f)
#endif

#if defined(DEBUG) && !defined(NDEBUG)
#   define CHECK_CURRENT_THREAD assert(::boost::green_thread::detail::thread_object::current_thread_)
#else
#   define CHECK_CURRENT_THREAD
#endif

namespace boost { namespace green_thread { namespace detail {
    typedef boost::asio::basic_waitable_timer<boost::chrono::steady_clock> timer_t;
    
    struct scheduler_object;
    typedef std::shared_ptr<scheduler_object> scheduler_ptr_t;
    
    struct thread_object;
    typedef std::shared_ptr<thread_object> thread_ptr_t;
    
    struct tss_cleanup_function;
    typedef const void *fss_key_t;
    typedef std::pair<std::shared_ptr<tss_cleanup_function>,void*> fss_value_t;
    
    typedef std::map<fss_key_t, fss_value_t> fss_map_t;
    
    struct thread_object : std::enable_shared_from_this<thread_object>, thread_base {
        enum state_t {
            READY,
            RUNNING,
            BLOCKED,
            STOPPED,
        };
        
        typedef std::deque<std::function<void()>> cleanup_queue_t;
        typedef boost::coroutines::coroutine<state_t>::pull_type runner_t;
        typedef boost::coroutines::coroutine<state_t>::push_type caller_t;
        typedef std::shared_ptr<boost::asio::strand> strand_ptr_t;
        
        thread_object(scheduler_ptr_t sched, thread_data_base *entry);
        thread_object(scheduler_ptr_t sched, strand_ptr_t strand, thread_data_base *entry);
        ~thread_object();
        
        void set_name(const std::string &s);
        std::string get_name();
        
        void raw_set_state(state_t s)
        { state_=s; }
        
        void set_state(state_t s) {
            if (caller_) {
                // We're in thread context, switch to scheduler context to make state take effect
                (*caller_)(s);
            } else {
                // We're in scheduler context
                state_=s;
            }
        }

        virtual void pause() override;
        virtual void activate() override;
        virtual void resume() override;
        virtual boost::asio::strand &get_thread_strand() override;
        
        // Following functions can only be called inside coroutine
        void yield(thread_ptr_t hint=thread_ptr_t());
        void join(thread_ptr_t f);
        void join_and_rethrow(thread_ptr_t f);
        void sleep_rel(duration_t d);
        
        // Implementations
        void runner_wrapper(caller_t &c);
        void one_step();
        
        void detach();
        
        void add_cleanup_function(std::function<void()> &&f);
        
        scheduler_ptr_t get_scheduler() override { return sched_; }
        
        static thread_object *& get_current_thread_object() {
            static THREAD_LOCAL thread_object *current_thread_=0;
            return current_thread_;
        }

        scheduler_ptr_t sched_;
        strand_ptr_t thread_strand_;
        mutable spinlock mtx_;
        boost::atomic<state_t> state_;
        std::unique_ptr<thread_data_base> entry_;
        runner_t runner_;
        caller_t *caller_;
        cleanup_queue_t cleanup_queue_;
        cleanup_queue_t join_queue_;
        fss_map_t fss_;
        thread_ptr_t this_ref_;
        std::string name_;
        std::exception_ptr uncaught_exception_;
        
        // Interruption support
        void interrupt();
        int interrupt_disable_level_=0;
        bool interrupt_requested_=false;
    };
    
    template<typename Lockable>
    struct relock_guard {
        inline relock_guard(Lockable &mtx)
        : mtx_(mtx)
        { mtx_.unlock(); }
        
        inline ~relock_guard()
        { mtx_.lock(); }
        
        Lockable &mtx_;
    };
}}} // End of namespace boost::green_thread::detail

namespace boost { namespace green_thread {
    inline detail::thread_object *current_thread_object() noexcept {
        return detail::thread_object::get_current_thread_object();
    }
    
    inline detail::thread_ptr_t current_thread_ptr() {
        CHECK_CURRENT_THREAD;
        return current_thread_object()->shared_from_this();
    }
}}  // End of namespace boost::green_thread

#endif
