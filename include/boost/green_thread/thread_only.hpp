//
//  thread_only.hpp
//  Boost.GreenThread
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// Copyright (c) 2015 Chen Xu
//

#ifndef BOOST_GREEN_THREAD_THREAD_ONLY_HPP
#define BOOST_GREEN_THREAD_THREAD_ONLY_HPP

#include <memory>
#include <functional>
#include <utility>
#include <type_traits>
#include <boost/green_thread/detail/config.hpp>
#include <boost/chrono/system_clocks.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/strand.hpp>
#include <boost/green_thread/detail/forward.hpp>
#include <boost/green_thread/detail/thread_data.hpp>

namespace boost { namespace green_thread {
    /// struct scheduler
    class BOOST_GREEN_THREAD_DECL scheduler {
    public:
        /// constructor
        scheduler();
        
        /**
         * returns the io_service associated with the scheduler
         */
        boost::asio::io_service &get_io_service();
        
        /**
         * starts the scheduler with in a worker thread pool with specific size
         */
        void start(size_t nthr=1);

        /**
         * waits until the scheduler object stops
         */
        void join();
        
        /**
         * add work threads into the worker thread pool
         */
        void add_worker_thread(size_t nthr=1);
        
        /**
         * returns number of threads in the worker pool
         */
        size_t worker_pool_size() const;
        
        /**
         * returns the scheduler singleton
         */
        static scheduler get_instance();

        scheduler(std::shared_ptr<detail::scheduler_object>);
    private:
        std::shared_ptr<detail::scheduler_object> impl_;
        friend class thread;
    };
    
    /// struct thread
    /**
     * manages a separate thread
     */
    class BOOST_GREEN_THREAD_DECL thread {
    public:
        /// thread id type
        typedef uintptr_t id;
        
        /// thread attributes
        struct attributes {
            // thread scheduling policy
            /**
             * A thread can be scheduled in two ways:
             * - normal: the thread is freely scheduled across all
             *           worker threads
             * - stick_with_parent: the thread always runs in the
             *                      same worker thread as its parent,
             *                      this can make sure the thread and
             *                      its parent never run concurrently,
             *                      avoid some synchronizations for
             *                      shares resources
             */
            enum scheduling_policy {
                /**
                 * scheduled freely in this scheduler
                 */
                normal,

                /**
                 * always runs in the same thread with parent
                 */
                stick_with_parent,
            } policy;
            
            /// constructor
            constexpr attributes(scheduling_policy p) : policy(p) {}
        };
        
        /// Constructs new thread object
        /**
         * Creates new thread object which does not represent a thread
         */
        thread() = default;
        
        /// Move constructor
        /**
         * Constructs the thread object to represent the thread of
         * execution that was represented by other. After this
         * call other no longer represents a thread of execution.
         */
        thread(thread &&other) noexcept;

        /**
         * Creates new thread object and associates it with a thread of execution.
         */
        template <class F>
        explicit thread(F &&f)
        : data_(detail::make_thread_data(utility::decay_copy(std::forward<F>(f))))
        { start(); }
        
        /**
         * Creates new thread object and associates it with a thread of execution.
         */
        template <class F, class Arg, class ...Args>
        thread(F&& f, Arg&& arg, Args&&... args)
        : data_(detail::make_thread_data(utility::decay_copy(std::forward<F>(f)),
                                        utility::decay_copy(std::forward<Arg>(arg)),
                                        utility::decay_copy(std::forward<Args>(args))...)
                )
        
        { start(); }
        
        /**
         * Creates new thread object and associates it with a thread of execution, using specific attributes
         */
        template <class F>
        thread(attributes attrs, F &&f)
        : data_(detail::make_thread_data(utility::decay_copy(std::forward<F>(f))))
        { start(attrs); }
        
        template <class F, class Arg, class ...Args>
        thread(attributes attrs, F&& f, Arg&& arg, Args&&... args)
        : data_(detail::make_thread_data(utility::decay_copy(std::forward<F>(f)),
                                        utility::decay_copy(std::forward<Arg>(arg)),
                                        utility::decay_copy(std::forward<Args>(args))...)
                )
        { start(attrs); }
        
        /**
         * Creates new thread object and associates it with a thread of execution in a specific scheduler.
         */
        template <class F>
        explicit thread(scheduler &sched, F &&f)
        : data_(detail::make_thread_data(utility::decay_copy(std::forward<F>(f))))
        { start(sched); }
        
        /**
         * Creates new thread object and associates it with a thread of execution in a specific scheduler.
         */
        template <class F, class Arg, class ...Args>
        thread(scheduler &sched, F&& f, Arg&& arg, Args&&... args)
        : data_(detail::make_thread_data(utility::decay_copy(std::forward<F>(f)),
                                        utility::decay_copy(std::forward<Arg>(arg)),
                                        utility::decay_copy(std::forward<Args>(args))...)
                )
        
        { start(sched); }
        
        /// Assigns the state of other to `*this` using move semantics.
        /**
         * If *this still has an associated running thread (i.e. `joinable() == true`), `std::terminate()` is called.
         */
        thread& operator=(thread &&other) noexcept;

        /**
         * checks whether the thread is joinable, i.e. potentially running in parallel context
         */
        bool joinable() const noexcept;
        
        /**
         * returns the _id_ of the thread
         */
        id get_id() const noexcept;
        
        /**
         * returns the number of concurrent threads supported by the implementation
         */
        static unsigned hardware_concurrency() noexcept;
        
        /**
         * waits for a thread to finish its execution
         */
        void join(bool propagate_exception=false);

        /**
         * permits the thread to execute independently
         */
        void detach();
        
        /**
         * swaps two thread objects
         */
        void swap(thread &other) noexcept(true);
        
        /**
         * sets the name of the thread
         */
        void set_name(const std::string &s);
        
        /**
         * returns the name of the thread
         */
        std::string get_name();
        
        /**
         * Interrupt the thread with a thread_interrupted exception if the interruption is not disabled.
         */
        void interrupt();
        
    private:
        /// non-copyable
        thread(const thread&) = delete;
        void start();
        void start(attributes);
        void start(scheduler &sched);

        std::unique_ptr<detail::thread_data_base> data_;
        std::shared_ptr<detail::thread_object> impl_;
    };
    
    constexpr thread::id not_a_thread=0;
    
    namespace this_thread {
        namespace detail {
            BOOST_GREEN_THREAD_DECL void sleep_rel(green_thread::detail::duration_t d);

            /**
             * returns the io_service associated with the current thread
             */
            BOOST_GREEN_THREAD_DECL boost::asio::io_service &get_io_service();
            
            /**
             * returns the strand associated with the current thread
             */
            BOOST_GREEN_THREAD_DECL boost::asio::strand &get_strand();
        }
        
        /**
         * reschedule execution of threads
         */
        BOOST_GREEN_THREAD_DECL void yield();
        
        /**
         * returns the thread id of the current thread
         */
        BOOST_GREEN_THREAD_DECL thread::id get_id();
        
        /**
         * indicates if the current running context is a thread
         */
        BOOST_GREEN_THREAD_DECL bool is_a_thread() noexcept(true);
        
        /**
         * stops the execution of the current thread for a specified time duration
         */
        template< class Rep, class Period >
        void sleep_for( const boost::chrono::duration<Rep,Period>& sleep_duration ) {
            detail::sleep_rel(boost::chrono::duration_cast<green_thread::detail::duration_t>(sleep_duration));
        }
        
        /**
         * stops the execution of the current thread until a specified time point
         */
        template< class Clock, class Duration >
        void sleep_until( const boost::chrono::time_point<Clock,Duration>& sleep_time ) {
            sleep_for(sleep_time-Clock::now());
        }
        
        /**
         * get the name of current thread
         */
        BOOST_GREEN_THREAD_DECL std::string get_name();

        /**
         * set the name of current thread
         */
        BOOST_GREEN_THREAD_DECL void set_name(const std::string &name);
        
        /**
         * register function which will be called on thread exit
         */
        BOOST_GREEN_THREAD_DECL void at_thread_exit(std::function<void()> &&f);
        
        /**
         * register function which will be called on thread exit
         */
        template<typename Fn>
        auto at_thread_exit(Fn &&fn)
        -> typename std::enable_if<!std::is_same<Fn, std::function<void()>>::value>::type
        {
            at_thread_exit(std::function<void()>(std::forward<Fn>(fn)));
        }
        
        /**
         * struct disable_interruption
         *
         * Create disable_interruption instance to increase interruption disable level.
         * Interruption is disable if the disable level larger than 0
         */
        struct BOOST_GREEN_THREAD_DECL disable_interruption {
            disable_interruption();
            ~disable_interruption();
        };
        
        /**
         * struct restore_interruption
         *
         * Create restore_interruption instance to decrease interruption disable level.
         * Interruption is not enable if the disable level larger than 0
         */
        struct BOOST_GREEN_THREAD_DECL restore_interruption {
            restore_interruption();
            ~restore_interruption();
        private:
            int level_=0;
        };
        
        /// Check if the interruption is enabled
        BOOST_GREEN_THREAD_DECL bool interruption_enabled() noexcept;

        /// Check if there is an interruption request
        BOOST_GREEN_THREAD_DECL bool interruption_requested() noexcept;
        
        /// Interruption request is checked only at this function call and some other predefined points
        BOOST_GREEN_THREAD_DECL void interruption_point();
    }   // End of namespace this_thread
    
    /**
     * get current scheduler
     */
    BOOST_GREEN_THREAD_DECL scheduler get_scheduler();

    namespace asio {
        using green_thread::this_thread::detail::get_io_service;
    }
}}  // End of namespace boost::green_thread

namespace std {
    /// specializes the std::swap algorithm for thread objects
    inline void swap(boost::green_thread::thread &lhs, boost::green_thread::thread &rhs) noexcept(true) {
        lhs.swap(rhs);
    }
}   // End of namespace std

#endif /* defined(boost_green_thread_thread_only_hpp) */
