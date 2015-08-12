//
//  barrier.h
//  Boost.GreenThread
//
// Copyright (C) 2002-2003
// David Moore, William E. Kempf
// Copyright (C) 2007-8 Anthony Williams
// Copyright (C) 2015 Chen Xu
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GREEN_THREAD_BARRIER_H
#define BOOST_GREEN_THREAD_BARRIER_H

#include <type_traits>
#include <boost/green_thread/exceptions.hpp>
#include <boost/green_thread/mutex.hpp>
#include <boost/green_thread/condition_variable.hpp>

namespace boost { namespace green_thread {
    namespace detail {
        typedef std::function<void()> void_completion_function;
        typedef std::function<size_t()> size_completion_function;
        struct default_barrier_reseter {
            unsigned int size_;
            default_barrier_reseter(unsigned int size) :
            size_(size) {}
            unsigned int operator()() {
                return size_;
            }
        };
        
        struct void_functor_barrier_reseter {
            unsigned int size_;
            void_completion_function fct_;
            template <typename F>
            void_functor_barrier_reseter(unsigned int size, F &&funct)
            : size_(size)
            , fct_(std::move(funct))
            {}

            unsigned int operator()() {
                fct_();
                return size_;
            }
        };
        
        struct void_fct_ptr_barrier_reseter {
            unsigned int size_;
            void(*fct_)();
            void_fct_ptr_barrier_reseter(unsigned int size, void(*funct)())
            : size_(size)
            , fct_(funct)
            {}
            unsigned int operator()() {
                fct_();
                return size_;
            }
        };
    }   // End of namespace boost::green_thread::detail

    /**
     * A barrier, also known as a rendezvous, is a synchronization point between multiple threads.
     * The barrier is configured for a particular number of threads (`n`), and as threads reach the
     * barrier they must wait until all `n` threads have arrived. Once the n-th thread has reached
     * the barrier, all the waiting threads can proceed, and the barrier is reset.
     */
    class barrier {
        struct dummy {};
    public:
        /**
         * Construct a barrier for `count` threads
         */
        explicit barrier(unsigned int count)
        : m_count(check_counter(count))
        , m_generation(0)
        , fct_(detail::default_barrier_reseter(count))
        {}
        
        /**
         * Construct a barrier for `count` threads and a completion function `completion`.
         */
        template <typename F>
        barrier(unsigned int count,
                F&& completion,
                typename std::enable_if<std::is_void<typename std::result_of<F>::type>::value, dummy*>::type=0)
        : m_count(check_counter(count))
        , m_generation(0)
        , fct_(detail::void_functor_barrier_reseter(count, std::move(completion)))
        {}
        
        /**
         * Construct a barrier for `count` threads and a completion function `completion`.
         */
        template <typename F>
        barrier(unsigned int count,
                F&& completion,
                typename std::enable_if<std::is_same<typename std::result_of<F>::type, unsigned int>::value, dummy*>::type=0)
        : m_count(check_counter(count))
        , m_generation(0)
        , fct_(std::move(completion))
        {}
        
        /**
         * Construct a barrier for `count` threads and a completion function `completion`.
         */
        barrier(unsigned int count, void(*completion)()) :
        m_count(check_counter(count)), m_generation(0),
        fct_(completion
             ? detail::size_completion_function(detail::void_fct_ptr_barrier_reseter(count, completion))
             : detail::size_completion_function(detail::default_barrier_reseter(count)))
        {}
        
        /**
         * Construct a barrier for `count` threads and a completion function `completion`.
         */
        barrier(unsigned int count, unsigned int(*completion)()) :
        m_count(check_counter(count)), m_generation(0),
        fct_(completion
             ? detail::size_completion_function(completion)
             : detail::size_completion_function(detail::default_barrier_reseter(count)))
        {}
        
        /**
         * Block until count threads have called `wait` or `count_down_and_wait` on `*this`.
         * When the count-th thread calls `wait`, the barrier is reset and all waiting threads
         * are unblocked. The reset depends on whether the barrier was constructed with a
         * completion function or not. If there is no completion function or if the completion
         * function result is `void`, the reset consists in restoring the original count.
         * Otherwise the rest consist in assigning the result of the completion function
         * (which must not be 0).
         */
        bool wait() {
            boost::unique_lock < mutex > lock(m_mutex);
            unsigned int gen = m_generation;
            
            if (--m_count == 0) {
                m_generation++;
                m_count = static_cast<unsigned int>(fct_());
                assert(m_count != 0);
                m_cond.notify_all();
                return true;
            }
            
            while (gen == m_generation)
                m_cond.wait(lock);
            return false;
        }
        
        /**
         * Block until count threads have called `wait` or `count_down_and_wait` on `*this`.
         * When the count-th thread calls `wait`, the barrier is reset and all waiting threads
         * are unblocked. The reset depends on whether the barrier was constructed with a
         * completion function or not. If there is no completion function or if the completion
         * function result is `void`, the reset consists in restoring the original count.
         * Otherwise the rest consist in assigning the result of the completion function
         * (which must not be 0).
         */
        void count_down_and_wait() {
            wait();
        }
        
    private:
        barrier(const barrier &)=delete;
        void operator=(const barrier &)=delete;
        
        static inline unsigned int check_counter(unsigned int count) {
            if (count == 0)
                BOOST_THROW_EXCEPTION(thread_exception(boost::system::errc::invalid_argument,
                                      "barrier constructor: count cannot be zero."));
            return count;
        }
        
        mutex m_mutex;
        condition_variable m_cond;
        unsigned int m_count;
        unsigned int m_generation;
        detail::size_completion_function fct_;
    };
}}  // End of namespace boost::green_thread

#endif
