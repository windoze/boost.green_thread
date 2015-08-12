//
//  mutex.cpp
//  Boost.GreenThread
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// Copyright (c) 2015 Chen Xu
//

#include <boost/thread/lock_guard.hpp>
#include <boost/green_thread/mutex.hpp>
#include "thread_object.hpp"

namespace boost { namespace green_thread {
    static const auto NOPERM=lock_error(boost::system::errc::operation_not_permitted);
    static const auto DEADLOCK=lock_error(boost::system::errc::resource_deadlock_would_occur);

    namespace detail {
        inline detail::thread_ptr_t cur_thread() {
            auto cf=current_thread_object();
            if (cf) {
                return cf->shared_from_this();
            }
            return detail::thread_ptr_t();
        }
    }
    
    void mutex::lock() {
        auto tf=detail::cur_thread();
        if (!tf) return;
        boost::lock_guard<detail::spinlock> lock(mtx_);
        if (owner_==tf) {
            BOOST_THROW_EXCEPTION(DEADLOCK);
        } else if(!owner_) {
            // This mutex is not locked
            // Acquire the mutex
            owner_=tf;
            return;
        }
        // This mutex is locked
        // Add this thread into waiting queue
        suspended_.push_back(tf);

        { detail::relock_guard<detail::spinlock> relock(mtx_); tf->pause(); }
    }
    
    void mutex::unlock() {
        auto tf=detail::cur_thread();
        if (!tf) return;
        boost::lock_guard<detail::spinlock> lock(mtx_);
        if (owner_!=tf) {
            // This thread doesn't own the mutex
            BOOST_THROW_EXCEPTION(NOPERM);
        }
        if (suspended_.empty()) {
            // Nobody is waiting
            owner_.reset();
            return;
        }
        // Set new owner and remove it from suspended queue
        std::swap(owner_, suspended_.front());
        suspended_.pop_front();
        owner_->resume();

        { detail::relock_guard<detail::spinlock> relock(mtx_); tf->yield(owner_); }
    }
    
    bool mutex::try_lock() {
        auto tf=detail::cur_thread();
        if (!tf) return false;
        boost::lock_guard<detail::spinlock> lock(mtx_);
        if (owner_==tf) {
            // This thread already owns the mutex
        } else if(!owner_) {
            // This mutex is not locked
            // Acquire the mutex
            owner_=tf;
        }
        // Return true if this thread owns the mutex
        return owner_==tf;
    }
    
    void recursive_mutex::lock() {
        auto tf=detail::cur_thread();
        if (!tf) return;
        boost::lock_guard<detail::spinlock> lock(mtx_);
        if (owner_==tf) {
            ++level_;
            return;
        } else if(!owner_) {
            // This mutex is not locked
            // Acquire the mutex
            assert(suspended_.empty());
            assert(level_==0);
            owner_=tf;
            level_=1;
            return;
        }
        // This mutex is locked
        // Add this thread into waiting queue
        suspended_.push_back(tf);
        
        { detail::relock_guard<detail::spinlock> relock(mtx_); tf->pause(); }
    }
    
    void recursive_mutex::unlock() {
        auto tf=detail::cur_thread();
        if (!tf) return;
        boost::lock_guard<detail::spinlock> lock(mtx_);
        if (owner_!=tf) {
            // This thread doesn't own the mutex
            BOOST_THROW_EXCEPTION(NOPERM);
        }
        --level_;
        if (level_>0) {
            // This thread still owns the mutex
            return;
        }
        if (suspended_.empty()) {
            // Nobody is waiting
            owner_.reset();
            assert(level_==0);
            return;
        }
        // Set new owner and remove it from suspended queue
        std::swap(owner_, suspended_.front());
        suspended_.pop_front();
        level_=1;
        owner_->resume();
        
        { detail::relock_guard<detail::spinlock> relock(mtx_); tf->yield(owner_); }
    }
    
    bool recursive_mutex::try_lock() {
        auto tf=detail::cur_thread();
        if (!tf) return false;
        boost::lock_guard<detail::spinlock> lock(mtx_);
        if (owner_==tf) {
            // This thread already owns the mutex, increase recursive level
            ++level_;
        } else if(!owner_) {
            // This mutex is not locked
            // Acquire the mutex
            assert(suspended_.empty());
            assert(level_==0);
            owner_=tf;
            level_=1;
        }
        // Cannot acquire the lock now
        return owner_==tf;
    }
    
    void timed_mutex::lock() {
        auto tf=detail::cur_thread();
        if (!tf) return;
        boost::lock_guard<detail::spinlock> lock(mtx_);
        if (owner_==tf) {
            BOOST_THROW_EXCEPTION(DEADLOCK);
        } else if(!owner_) {
            // This mutex is not locked
            // Acquire the mutex
            owner_=tf;
            return;
        }
        // This mutex is locked
        // Add this thread into waiting queue without attached timer
        suspended_.push_back({tf, 0});
        
        { detail::relock_guard<detail::spinlock> relock(mtx_); tf->pause(); }
    }
    
    bool timed_mutex::try_lock() {
        auto tf=detail::cur_thread();
        if (!tf) return false;
        boost::lock_guard<detail::spinlock> lock(mtx_);
        if (owner_==tf) {
            // This thread already owns the mutex
            return true;
        } else if(!owner_) {
            // This mutex is not locked
            // Acquire the mutex
            owner_=tf;
            return true;
        }
        // Cannot acquire the lock now
        return false;
    }
    
    void timed_mutex::unlock() {
        auto tf=detail::cur_thread();
        if (!tf) return;
        boost::lock_guard<detail::spinlock> lock(mtx_);
        if (owner_!=tf) {
            // This thread doesn't own the mutex
            BOOST_THROW_EXCEPTION(NOPERM);
        }
        if (suspended_.empty()) {
            // Nobody is waiting
            owner_.reset();
            return;
        }
        // Set new owner and remove it from suspended queue
        std::swap(owner_, suspended_.front().f_);
        detail::timer_t *t=suspended_.front().t_;
        suspended_.pop_front();
        if (t) {
            // Cancel attached timer, the timer handler will schedule new owner
            t->cancel();
        } else {
            // No attached timer, directly schedule new owner
            owner_->resume();
        }
        
        { detail::relock_guard<detail::spinlock> relock(mtx_); tf->yield(owner_); }
    }
    
    void timed_mutex::timeout_handler(detail::thread_ptr_t this_thread,
                                      boost::system::error_code ec)
    {
        boost::lock_guard<detail::spinlock> lock(mtx_);
        if (owner_!=this_thread) {
            // This thread doesn't own the mutex
            // Find and remove this thread from waiting queue
            auto i=std::find(suspended_.begin(),
                             suspended_.end(),
                             this_thread);
            if (i!=suspended_.end()) {
                suspended_.erase(i);
            }
        } else {
            // This thread should not be in the suspended queue, do nothing
        }
        this_thread->resume();
    }
    
    bool timed_mutex::try_lock_rel(detail::duration_t d) {
        auto tf=detail::cur_thread();
        if (!tf) return false;
        boost::lock_guard<detail::spinlock> lock(mtx_);
        if (owner_==tf) {
            BOOST_THROW_EXCEPTION(DEADLOCK);
        } else if(!owner_) {
            // This mutex is not locked
            // Acquire the mutex
            owner_=tf;
            return true;
        }
        // This mutex is locked
        // Add this thread into waiting queue
        detail::timer_t t(tf->get_io_service());
        t.expires_from_now(d);
        t.async_wait(tf->get_thread_strand().wrap(std::bind(&timed_mutex::timeout_handler,
                                                                    this,
                                                                    tf,
                                                                    std::placeholders::_1)));
        suspended_.push_back({tf, &t});
        
        // This thread will be resumed when timer triggered/canceled or other called unlock()
        { detail::relock_guard<detail::spinlock> relock(mtx_); tf->pause(); }
        
        return owner_==tf;
    }

    void recursive_timed_mutex::lock() {
        auto tf=detail::cur_thread();
        if (!tf) return;
        boost::lock_guard<detail::spinlock> lock(mtx_);
        if (owner_==tf) {
            ++level_;
            return;
        } else if(!owner_) {
            // This mutex is not locked
            // Acquire the mutex
            owner_=tf;
            level_=1;
            return;
        }
        // This mutex is locked
        // Add this thread into waiting queue without attached timer
        suspended_.push_back({tf, 0});
        
        { detail::relock_guard<detail::spinlock> relock(mtx_); tf->pause(); }
    }
    
    void recursive_timed_mutex::unlock() {
        auto tf=detail::cur_thread();
        if (!tf) return;
        boost::lock_guard<detail::spinlock> lock(mtx_);
        if (owner_!=tf) {
            // This thread doesn't own the mutex
            BOOST_THROW_EXCEPTION(NOPERM);
        }
        --level_;
        if(level_>0) {
            // This thread still owns the mutex
            return;
        }
        if (suspended_.empty()) {
            // Nobody is waiting
            owner_.reset();
            level_=0;
            return;
        }
        // Set new owner and remove it from suspended queue
        std::swap(owner_, suspended_.front().f_);
        detail::timer_t *t=suspended_.front().t_;
        suspended_.pop_front();
        level_=1;
        if (t) {
            // Cancel attached timer, the timer handler will schedule new owner
            t->cancel();
        } else {
            // No attached timer, directly schedule new owner
            owner_->resume();
        }
        
        { detail::relock_guard<detail::spinlock> relock(mtx_); tf->yield(owner_); }
    }
    
    bool recursive_timed_mutex::try_lock() {
        auto tf=detail::cur_thread();
        if (!tf) return false;
        boost::lock_guard<detail::spinlock> lock(mtx_);
        if (owner_==tf) {
            // This thread already owns the mutex, increase recursive level
            ++level_;
        } else if(!owner_) {
            // This mutex is not locked
            // Acquire the mutex
            owner_=tf;
            level_=1;
        }
        // Cannot acquire the lock now
        return owner_==tf;
    }
    
    void recursive_timed_mutex::timeout_handler(detail::thread_ptr_t this_thread, boost::system::error_code ec) {
        boost::lock_guard<detail::spinlock> lock(mtx_);
        if (owner_!=this_thread) {
            // This thread doesn't own the mutex
            // Find and remove this thread from waiting queue
            auto i=std::find(suspended_.begin(),
                             suspended_.end(),
                             this_thread);
            if (i!=suspended_.end()) {
                suspended_.erase(i);
            }
        } else {
            // This thread should not be in the suspended queue, do nothing
        }
        this_thread->resume();
    }
    
    bool recursive_timed_mutex::try_lock_rel(detail::duration_t d) {
        auto tf=detail::cur_thread();
        if (!tf) return false;
        boost::lock_guard<detail::spinlock> lock(mtx_);
        if (owner_==tf) {
            ++level_;
            return true;
        } else if(!owner_) {
            // This mutex is not locked
            // Acquire the mutex
            owner_=tf;
            level_=1;
            return true;
        }
        // This mutex is locked
        // Add this thread into waiting queue
        detail::timer_t t(tf->get_io_service());
        t.expires_from_now(d);
        t.async_wait(tf->get_thread_strand().wrap(std::bind(&recursive_timed_mutex::timeout_handler,
                                                                    this,
                                                                    tf,
                                                                    std::placeholders::_1)));
        suspended_.push_back({tf, &t});
        
        // This thread will be resumed when timer triggered/canceled or other called unlock()
        { detail::relock_guard<detail::spinlock> relock(mtx_); tf->pause(); }
        return owner_==tf;
    }
}}  // End of namespace boost::green_thread
