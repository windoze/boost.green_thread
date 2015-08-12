//
//  condition.cpp
//  Boost.GreenThread
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// Copyright (c) 2015 Chen Xu
//

#include <boost/system/error_code.hpp>
#include <boost/thread/lock_guard.hpp>
#include <boost/green_thread/condition_variable.hpp>
#include "thread_object.hpp"

namespace boost { namespace green_thread {
    static const auto NOPERM=condition_error(boost::system::errc::operation_not_permitted);
    
    void condition_variable::wait(boost::unique_lock<mutex>& lock) {
        auto tf=current_thread_ptr();
        mutex *m=lock.mutex();
        if (tf!=m->owner_) {
            // This thread doesn't own the mutex
            BOOST_THROW_EXCEPTION(NOPERM);
        }
        {
            boost::lock_guard<detail::spinlock> lock(mtx_);
            // The "suspension of this thread" is actually happened here, not the pause()
            // as other will see there is a thread in the waiting queue.
            suspended_.push_back(suspended_item({m, tf, 0}));
        }
        { detail::relock_guard<mutex> relock(*m); tf->pause(); }
    }
    
    void condition_variable::timeout_handler(detail::thread_ptr_t this_thread,
                                             detail::timer_t *t,
                                             cv_status &ret,
                                             boost::system::error_code ec)
    {
        if(!ec) {
            // Timeout
            // Timeout handler, find and remove this thread from waiting queue
            boost::lock_guard<detail::spinlock> lock(mtx_);
            ret=cv_status::timeout;
            // Find and remove this thread from waiting queue
            auto i=std::find(suspended_.begin(),
                             suspended_.end(),
                             this_thread);
            if (i!=suspended_.end()) {
                suspended_.erase(i);
            }
        }
        this_thread->resume();
    }
    
    cv_status condition_variable::wait_rel(boost::unique_lock<mutex>& lock, detail::duration_t d) {
        auto tf=current_thread_ptr();
        mutex *m=lock.mutex();
        cv_status ret=cv_status::no_timeout;
        if (tf!=m->owner_) {
            // This thread doesn't own the mutex
            BOOST_THROW_EXCEPTION(NOPERM);
        }
        detail::timer_t t(tf->get_io_service());
        {
            boost::lock_guard<detail::spinlock> lock(mtx_);
            suspended_.push_back(suspended_item({m, tf, &t}));
            t.expires_from_now(d);
            t.async_wait(tf->get_thread_strand().wrap(std::bind(&condition_variable::timeout_handler,
                                                                        this,
                                                                        tf,
                                                                        &t,
                                                                        std::ref(ret),
                                                                        std::placeholders::_1)));
        }
        { detail::relock_guard<mutex> relock(*m); tf->pause(); }
        return ret;
    }
    
    void condition_variable::notify_one() {
        {
            boost::lock_guard<detail::spinlock> lock(mtx_);
            if (suspended_.empty()) {
                return;
            }
            suspended_item p(suspended_.front());
            suspended_.pop_front();
            if (p.t_) {
                // Cancel attached timer if it's set
                // Timer handler will reschedule the waiting thread
                p.t_->cancel();
                p.t_=0;
            } else {
                // No timer attached to the waiting thread, directly schedule it
                p.f_->resume();
            }
        }
        // Only yield if currently in a thread
        // CV can be used to notify a thread from not-a-thread, i.e. foreign thread
        if (auto cf=current_thread_object()) {
            cf->yield();
        }
    }
    
    void condition_variable::notify_all() {
        {
            boost::lock_guard<detail::spinlock> lock(mtx_);
            while (!suspended_.empty()) {
                suspended_item p(suspended_.front());
                suspended_.pop_front();
                if (p.t_) {
                    // Cancel attached timer if it's set
                    // Timer handler will reschedule the waiting thread
                    p.t_->cancel();
                } else {
                    // No timer attached to the waiting thread, directly schedule it
                    p.f_->resume();
                }
            }
        }
        // Only yield if currently in a thread
        // CV can be used to notify a thread from not-a-thread, i.e. foreign thread
        if (auto cf=current_thread_object()) {
            cf->yield();
        }
    }

    struct cleanup_handler {
        condition_variable &c_;
        boost::unique_lock<mutex> l_;
        
        cleanup_handler(condition_variable &cond,
                        boost::unique_lock<mutex> lk)
        : c_(cond)
        , l_(std::move(lk))
        {}
        
        void operator()() {
            l_.unlock();
            c_.notify_all();
        }
    };
    
    inline void run_cleanup_handler(cleanup_handler *h) {
        std::unique_ptr<cleanup_handler> ch(h);
        (*ch)();
    }
    
    void notify_all_at_thread_exit(condition_variable &cond,
                                   boost::unique_lock<mutex> lk)
    {
        CHECK_CURRENT_THREAD;
        if (auto cf=current_thread_object()) {
            cleanup_handler *h=new cleanup_handler(cond, std::move(lk));
            cf->add_cleanup_function(std::bind(run_cleanup_handler, h));
        }
    }
}}  // End of namespace boost::green_thread
