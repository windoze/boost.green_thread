//
//  use_future.hpp
//  Boost.GreenThread
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// Copyright (c) 2015 Chen Xu
//

#ifndef BOOST_GREEN_THREAD_ASIO_DETAIL_USE_FUTURE_HPP
#define BOOST_GREEN_THREAD_ASIO_DETAIL_USE_FUTURE_HPP

#include <memory>

#include <boost/asio/async_result.hpp>
#include <boost/asio/detail/config.hpp>
#include <boost/asio/handler_type.hpp>
#include <boost/make_shared.hpp>
#include <boost/move/move.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/system/error_code.hpp>
#include <boost/system/system_error.hpp>
#include <boost/thread/detail/memory.hpp>
#include <boost/throw_exception.hpp>

#include <boost/green_thread/future/future.hpp>
#include <boost/green_thread/future/promise.hpp>

namespace boost { namespace green_thread { namespace asio { namespace detail {
    // Completion handler to adapt a promise as a completion handler.
    template<typename T>
    class promise_handler
    {
    public:
        // Construct from use_future special value.
        template<typename Allocator>
        promise_handler(asio::use_future_t<Allocator> uf)
        : promise_(std::allocate_shared<promise<T>>(uf.get_allocator(),
                                                    boost::allocator_arg,
                                                    uf.get_allocator()))
        , thread_(boost::green_thread::detail::get_current_thread_ptr())
        {}
        
        void operator()(T t)
        {
            promise_->set_value(t);
        }
        
        void operator()(const boost::system::error_code &ec, T t)
        {
            if (ec)
                promise_->set_exception(utility::copy_exception(boost::system::system_error(ec)));
            else
                promise_->set_value(t);
        }
        
        //private:
        std::shared_ptr<promise<T>> promise_;
        boost::green_thread::detail::thread_base::ptr_t thread_;
    };
    
    template<>
    class promise_handler<void>
    {
    public:
        // Construct from use_future special value. Used during rebinding.
        template<typename Allocator>
        promise_handler(use_future_t<Allocator> uf)
        : promise_(std::allocate_shared<promise<void>>(uf.get_allocator(),
                                                       boost::allocator_arg,
                                                       uf.get_allocator()))
        , thread_(boost::green_thread::detail::get_current_thread_ptr())
        {}
        
        void operator()()
        {
            promise_->set_value();
        }
        
        void operator()(const boost::system::error_code &ec)
        {
            if (ec)
                promise_->set_exception(utility::copy_exception(boost::system::system_error(ec)));
            else
                promise_->set_value();
        }
        
        //private:
        std::shared_ptr<promise<void>> promise_;
        boost::green_thread::detail::thread_base::ptr_t thread_;
    };

    // Ensure any exceptions thrown from the handler are propagated back to the
    // caller via the future.
    template<typename Function, typename T>
    void do_handler_invoke(Function f, boost::green_thread::asio::detail::promise_handler<T> h)
    {
        std::shared_ptr<boost::green_thread::promise<T>> p(h.promise_);
        try
        { f(); }
        catch (...)
        { p->set_exception(std::current_exception()); }
    }
}}}}    // End of namespace boost::green_thread::asio::detail

namespace boost { namespace asio {
    namespace detail {
        template<typename Function, typename T>
        void asio_handler_invoke(Function f, boost::green_thread::asio::detail::promise_handler<T> *h)
        {
            boost::green_thread::scheduler s(h->thread_->get_scheduler());
            boost::green_thread::thread(s,
                         boost::green_thread::asio::detail::do_handler_invoke<Function, T>,
                         f,
                         boost::green_thread::asio::detail::promise_handler<T>(std::move(*h))).detach();
        }
    }   // End of namespace boost::asio::detail
    
    // Handler traits specialisation for promise_handler.
    template<typename T>
    class async_result<boost::green_thread::asio::detail::promise_handler<T>>
    {
    public:
        // The initiating function will return a future.
        typedef boost::green_thread::future<T> type;
        
        // Constructor creates a new promise for the async operation, and obtains the
        // corresponding future.
        explicit async_result(boost::green_thread::asio::detail::promise_handler<T> &h)
        { value_ = h.promise_->get_future(); }
        
        // Obtain the future to be returned from the initiating function.
        type get()
        { return std::move(value_); }
        
    private:
        type value_;
    };
    
    // Handler type specialisation for use_future.
    template<typename Allocator, typename ReturnType>
    struct handler_type<boost::green_thread::asio::use_future_t<Allocator>, ReturnType()>
    { typedef boost::green_thread::asio::detail::promise_handler<void> type; };
    
    // Handler type specialisation for use_future.
    template<typename Allocator, typename ReturnType, typename Arg1>
    struct handler_type<boost::green_thread::asio::use_future_t<Allocator>, ReturnType(Arg1)>
    { typedef boost::green_thread::asio::detail::promise_handler<Arg1> type; };
    
    // Handler type specialisation for use_future.
    template<typename Allocator, typename ReturnType>
    struct handler_type<boost::green_thread::asio::use_future_t<Allocator>, ReturnType(boost::system::error_code)>
    { typedef boost::green_thread::asio::detail::promise_handler<void> type; };
    
    // Handler type specialisation for use_future.
    template<typename Allocator, typename ReturnType, typename Arg2>
    struct handler_type<boost::green_thread::asio::use_future_t<Allocator>, ReturnType(boost::system::error_code, Arg2)>
    { typedef boost::green_thread::asio::detail::promise_handler<Arg2> type; };
}}  // End of namespace boost::asio

#endif
