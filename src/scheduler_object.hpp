//
//  scheduler_object.hpp
//  Boost.GreenThread
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// Copyright (c) 2015 Chen Xu
//

#ifndef BOOST_GREEN_THREAD_SCHEDULER_OBJECT_HPP
#define BOOST_GREEN_THREAD_SCHEDULER_OBJECT_HPP

#include <memory>
#include <vector>
#include <boost/asio/io_service.hpp>
#include <boost/thread/thread.hpp>
#include "thread_object.hpp"

namespace boost { namespace green_thread { namespace detail {
    struct scheduler_object : std::enable_shared_from_this<scheduler_object> {
        scheduler_object();
        thread_ptr_t make_thread(thread_data_base *entry);
        thread_ptr_t make_thread(std::shared_ptr<boost::asio::strand> s, thread_data_base *entry);
        void start(size_t nthr);
        void join();
        
        void add_thread(size_t nthr);
        size_t worker_pool_size() const;
        
        void on_thread_exit(thread_ptr_t p);
        void on_check_timer(boost::system::error_code ec);
        
        static std::shared_ptr<scheduler_object> get_instance();
        
        mutable boost::mutex mtx_;
        boost::condition_variable cv_;
        std::vector<boost::thread> threads_;
        boost::asio::io_service io_service_;
        boost::atomic<size_t> thread_count_;
        boost::atomic<bool> started_;
        std::unique_ptr<timer_t> check_timer;
        
        //static std::once_flag instance_inited_;
        //static std::shared_ptr<scheduler_object> the_instance_;
    };
}}} // End of namespace boost::green_thread::detail

#endif /* defined(boost_green_thread_scheduler_object_hpp) */
