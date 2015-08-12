//
//  yield.hpp
//  Boost.GreenThread
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// Copyright (c) 2015 Chen Xu
//

#ifndef BOOST_GREEN_THREAD_ASIO_DETAIL_YIELD_HPP
#define BOOST_GREEN_THREAD_ASIO_DETAIL_YIELD_HPP

#include <memory>
#include <boost/chrono/system_clocks.hpp>
#include <boost/system/error_code.hpp>
#include <boost/system/system_error.hpp>
#include <boost/asio/async_result.hpp>
#include <boost/asio/handler_invoke_hook.hpp>
#include <boost/asio/detail/config.hpp>
#include <boost/asio/error.hpp>
#include <boost/asio/handler_type.hpp>
#include <boost/throw_exception.hpp>
#include <boost/green_thread/detail/thread_base.hpp>

namespace boost { namespace green_thread { namespace asio { namespace detail {
    template<typename T>
    class yield_handler
    {
    public:
        yield_handler(const yield_t &y)
        : ec_(y.ec_)
        , value_(0)
        , thread_(boost::green_thread::detail::get_current_thread_ptr())
        {}
        
        void operator()(T t)
        {
            // Async op completed, resume waiting thread
            *ec_ = boost::system::error_code();
            *value_ = t;
            thread_->activate();
        }
        
        void operator()(const boost::system::error_code &ec, T t)
        {
            // Async op completed, resume waiting thread
            *ec_ = ec;
            *value_ = t;
            thread_->activate();
        }
        
        //private:
        boost::system::error_code *ec_;
        T *value_;
        boost::green_thread::detail::thread_base::ptr_t thread_;
    };
    
    // Completion handler to adapt a void promise as a completion handler.
    template<>
    class yield_handler<void>
    {
    public:
        yield_handler(const yield_t &y)
        : ec_(y.ec_)
        , thread_(boost::green_thread::detail::get_current_thread_ptr())
        {}
        
        void operator()()
        {
            // Async op completed, resume waiting thread
            *ec_ = boost::system::error_code();
            thread_->activate();
        }
        
        void operator()(boost::system::error_code const& ec)
        {
            // Async op completed, resume waiting thread
            *ec_ = ec;
            thread_->activate();
        }
        
        //private:
        boost::system::error_code *ec_;
        boost::green_thread::detail::thread_base::ptr_t thread_;
    };
}}}}    // End of namespace boost::green_thread::asio::detail

namespace boost { namespace asio {
    template<typename T>
    class async_result<boost::green_thread::asio::detail::yield_handler<T>>
    {
    public:
        typedef T type;
        
        explicit async_result(boost::green_thread::asio::detail::yield_handler<T> &h)
        {
            out_ec_ = h.ec_;
            if (!out_ec_) h.ec_ = & ec_;
            h.value_ = & value_;
        }
        
        type get()
        {
            // Wait until async op completed
            boost::green_thread::detail::get_current_thread_ptr()->pause();
            if (!out_ec_ && ec_)
                BOOST_THROW_EXCEPTION(boost::system::system_error(ec_));
            return value_;
        }
        
    private:
        
        boost::system::error_code *out_ec_;
        boost::system::error_code ec_;
        type value_;
    };

    template<>
    class async_result<boost::green_thread::asio::detail::yield_handler<void>>
    {
    public:
        typedef void  type;
        
        explicit async_result(boost::green_thread::asio::detail::yield_handler<void> &h)
        {
            out_ec_ = h.ec_;
            if (!out_ec_) h.ec_ = & ec_;
        }
        
        void get()
        {
            // Wait until async op completed
            boost::green_thread::detail::get_current_thread_ptr()->pause();
            if (!out_ec_ && ec_)
                BOOST_THROW_EXCEPTION(boost::system::system_error(ec_));
        }
        
    private:
        boost::system::error_code *out_ec_;
        boost::system::error_code ec_;
    };
    
    // Handler type specialisation for yield_t.
    template<typename ReturnType>
    struct handler_type<boost::green_thread::asio::yield_t, ReturnType()>
    { typedef boost::green_thread::asio::detail::yield_handler<void> type; };
    
    // Handler type specialisation for yield_t.
    template<typename ReturnType, typename Arg1>
    struct handler_type<boost::green_thread::asio::yield_t, ReturnType(Arg1)>
    { typedef boost::green_thread::asio::detail::yield_handler<Arg1> type; };
    
    // Handler type specialisation for yield_t.
    template<typename ReturnType>
    struct handler_type<boost::green_thread::asio::yield_t, ReturnType(boost::system::error_code)>
    { typedef boost::green_thread::asio::detail::yield_handler<void> type; };
    
    // Handler type specialisation for yield_t.
    template<typename ReturnType, typename Arg2>
    struct handler_type<boost::green_thread::asio::yield_t, ReturnType( boost::system::error_code, Arg2)>
    { typedef boost::green_thread::asio::detail::yield_handler<Arg2> type; };
}}  // End of namespace boost::asio
#endif
