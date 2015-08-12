//
//  spinlock.hpp
//  Boost.GreenThread
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// Copyright (c) 2015 Chen Xu
//

#ifndef BOOST_GREEN_THREAD_DETAIL_SPINLOCK_HPP
#define BOOST_GREEN_THREAD_DETAIL_SPINLOCK_HPP

#include <boost/atomic/atomic.hpp>
 
namespace boost { namespace green_thread { namespace detail {
    /**
     * class spinlock
     *
     * A spinlock meets C++11 BasicLockable concept
     */
    class spinlock {
    private:
        typedef enum {Locked, Unlocked} LockState;
        boost::atomic<LockState> state_;

    public:
        /// Constructor
        spinlock() noexcept : state_(Unlocked) {}
  
        /// Blocks until a lock can be obtained for the current execution agent.
        void lock() noexcept {
            while (state_.exchange(Locked, boost::memory_order_acquire) == Locked) {
                /* busy-wait */
            }
        }

        /// Releases the lock held by the execution agent.
        void unlock() noexcept {
            state_.store(Unlocked, boost::memory_order_release);
        }
    };
}}} // End of namespace boost::green_thread::detail

#endif