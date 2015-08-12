//
//  scheduler_object.cpp
//  Boost.GreenThread
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// Copyright (c) 2015 Chen Xu
//

#include <boost/thread/lock_types.hpp>
#include <boost/green_thread/thread_only.hpp>
#include "scheduler_object.hpp"

namespace boost { namespace green_thread { namespace detail {
    scheduler_object::scheduler_object()
    : thread_count_(0)
    , started_(false)
    {}
    
    thread_ptr_t scheduler_object::make_thread(thread_data_base *entry) {
        boost::lock_guard<boost::mutex> guard(mtx_);
        thread_count_++;
        thread_ptr_t ret(std::make_shared<thread_object>(shared_from_this(), entry));
        if (!started_) {
            started_=true;
        }
        ret->resume();
        return ret;
    }
    
    thread_ptr_t scheduler_object::make_thread(std::shared_ptr<boost::asio::strand> s, thread_data_base *entry) {
        boost::lock_guard<boost::mutex> guard(mtx_);
        thread_count_++;
        thread_ptr_t ret(std::make_shared<thread_object>(shared_from_this(), s, entry));
        if (!started_) {
            started_=true;
        }
        ret->resume();
        return ret;
    }
    
    static inline void run_in_this_thread(scheduler_ptr_t pthis) {
        pthis->io_service_.run();
    }
    
    void scheduler_object::start(size_t nthr) {
        boost::lock_guard<boost::mutex> guard(mtx_);
        if (threads_.size()>0) {
            // Already started
            return;
        }

        check_timer.reset(new timer_t(io_service_));
        check_timer->expires_from_now(boost::chrono::milliseconds(50));
        scheduler_ptr_t pthis(shared_from_this());
        check_timer->async_wait(std::bind(&scheduler_object::on_check_timer, pthis, std::placeholders::_1));
        for(size_t i=0; i<nthr; i++) {
            threads_.push_back(boost::thread(run_in_this_thread, pthis));
        }
    }
    
    void scheduler_object::join() {
        {
            // Wait until there is no running thread
            boost::unique_lock<boost::mutex> lock(mtx_);
            while (started_ && thread_count_>0) {
                cv_.wait(lock);
            }
        }
        
        // Join all worker threads
        for(boost::thread &t : threads_) {
            t.join();
        }
        threads_.clear();
        started_=false;
        io_service_.reset();
    }
    
    void scheduler_object::add_thread(size_t nthr) {
        boost::lock_guard<boost::mutex> guard(mtx_);
        scheduler_ptr_t pthis(shared_from_this());
        for(size_t i=0; i<nthr; i++) {
            threads_.push_back(boost::thread(std::bind(run_in_this_thread, pthis)));
        }
    }
    
    size_t scheduler_object::worker_pool_size() const {
        boost::lock_guard<boost::mutex> guard(mtx_);
        return threads_.size();
    }
    
    void scheduler_object::on_thread_exit(thread_ptr_t p) {
        boost::lock_guard<boost::mutex> guard(mtx_);
        thread_count_--;
        // Release this_ref for detached threads
        p->this_ref_.reset();
    }
    
    void scheduler_object::on_check_timer(boost::system::error_code ec) {
        boost::lock_guard<boost::mutex> guard(mtx_);
        if (thread_count_>0 || !started_) {
            check_timer->expires_from_now(boost::chrono::milliseconds(50));
            check_timer->async_wait(std::bind(&scheduler_object::on_check_timer, shared_from_this(), std::placeholders::_1));
        } else {
            io_service_.stop();
            cv_.notify_one();
        }
    }
    
    std::shared_ptr<scheduler_object> scheduler_object::get_instance() {
        static std::once_flag instance_inited_;
        static std::shared_ptr<scheduler_object> the_instance_;

        std::call_once(instance_inited_, [&](){
            the_instance_=std::make_shared<scheduler_object>();
        });
        return the_instance_;
    }
}}} // End of namespace boost::green_thread::detail

namespace boost { namespace green_thread {
    scheduler::scheduler()
    : impl_(std::make_shared<detail::scheduler_object>())
    {}
    
    scheduler::scheduler(std::shared_ptr<detail::scheduler_object> impl)
    : impl_(impl)
    {}
    
    boost::asio::io_service &scheduler::get_io_service()
    { return impl_->io_service_; }
    
    void scheduler::start(size_t nthr) {
        impl_->start(nthr);
    }
    
    void scheduler::join() {
        impl_->join();
    }
    
    void scheduler::add_worker_thread(size_t nthr) {
        impl_->add_thread(nthr);
    }
    
    size_t scheduler::worker_pool_size() const {
        return impl_->worker_pool_size();
    }
    
    scheduler scheduler::get_instance() {
        return scheduler(detail::scheduler_object::get_instance());
    }
}}  // End of namespace boost::green_thread
