//
//  exceptions.hpp
//  Boost.GreenThread
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// Copyright (c) 2015 Chen Xu
//

#ifndef BOOST_GREEN_THREAD_EXCEPTIONS_HPP
#define BOOST_GREEN_THREAD_EXCEPTIONS_HPP

#include <stdexcept>
#include <string>
#include <boost/green_thread/detail/config.hpp>
#include <boost/type_traits.hpp>
#include <boost/detail/scoped_enum_emulation.hpp>
#include <boost/system/error_code.hpp>
#include <boost/system/system_error.hpp>

namespace boost { namespace green_thread {
    class thread_exception : public boost::system::system_error {
    public:
        thread_exception()
        : boost::system::system_error(0, boost::system::system_category())
        {}
        
        thread_exception(int sys_error_code)
        : boost::system::system_error(sys_error_code, boost::system::system_category())
        {}
        
        thread_exception(int ev, const char *what_arg)
        : boost::system::system_error(boost::system::error_code(ev, boost::system::system_category()), what_arg)
        {}
        
        thread_exception(int ev, const std::string &what_arg)
        : boost::system::system_error(boost::system::error_code(ev, boost::system::system_category()), what_arg)
        {}
        
        ~thread_exception() noexcept
        {}
        
        int native_error() const
        { return code().value(); }
    };
    
    class condition_error : public thread_exception {
    public:
        condition_error()
        : thread_exception(0, "boost::green_thread::condition_error")
        {}
        
        condition_error(int ev)
        : thread_exception(ev, "boost::green_thread::condition_error")
        {}
        
        condition_error(int ev, const char *what_arg)
        : thread_exception(ev, what_arg)
        {}
        
        condition_error(int ev, const std::string &what_arg)
        : thread_exception(ev, what_arg)
        {}
    };
    
    class lock_error : public thread_exception {
    public:
        lock_error()
        : thread_exception(0, "boost::green_thread::lock_error")
        {}
        
        lock_error(int ev)
        : thread_exception(ev, "boost::green_thread::lock_error")
        {}
        
        lock_error(int ev, const char *what_arg)
        : thread_exception(ev, what_arg)
        {}
        
        lock_error(int ev, const std::string &what_arg)
        : thread_exception(ev, what_arg)
        {}
        
        ~lock_error() noexcept
        {}
    };
    
    class thread_resource_error : public thread_exception {
    public:
        thread_resource_error()
        : thread_exception(boost::system::errc::resource_unavailable_try_again,
                          "boost::green_thread::thread_resource_error")
        {}
        
        thread_resource_error(int ev)
        : thread_exception(ev, "boost::green_thread::thread_resource_error")
        {}
        
        thread_resource_error(int ev, const char *what_arg)
        : thread_exception(ev, what_arg)
        {}
        
        thread_resource_error(int ev, const std::string &what_arg)
        : thread_exception(ev, what_arg)
        {}
        
        ~thread_resource_error() noexcept
        {}
    };
    
    class invalid_argument : public thread_exception {
    public:
        invalid_argument()
        : thread_exception(boost::system::errc::invalid_argument,
                          "boost::green_thread::invalid_argument")
        {}
        
        invalid_argument(int ev)
        : thread_exception(ev, "boost::green_thread::invalid_argument")
        {}
        
        invalid_argument(int ev, const char *what_arg)
        : thread_exception(ev, what_arg)
        {}
        
        invalid_argument(int ev, const std::string &what_arg)
        : thread_exception(ev, what_arg)
        {}
    };
    
    class thread_interrupted : public thread_exception {
    public:
        thread_interrupted()
        : thread_exception(boost::system::errc::interrupted, "boost::green_thread::thread_interrupted")
        {}
    };
    
    enum class future_errc
    {
        broken_promise = 1,
        future_already_retrieved,
        promise_already_satisfied,
        no_state
    };
    
    BOOST_GREEN_THREAD_DECL boost::system::error_category const &future_category() noexcept;
}}  // End of namespace boost::green_thread

namespace boost { namespace system {
    template<>
    struct is_error_code_enum<boost::green_thread::future_errc> : public true_type
    {};
    
    inline error_code make_error_code(boost::green_thread::future_errc e) //noexcept
    {
        return error_code(underlying_cast<int>(e), boost::green_thread::future_category());
    }
    
    inline error_condition make_error_condition(boost::green_thread::future_errc e) //noexcept
    {
        return error_condition(underlying_cast<int>(e), boost::green_thread::future_category());
    }
}}  // End of namespace boost::system

namespace boost { namespace green_thread {
    class future_error : public std::logic_error {
    private:
        boost::system::error_code ec_;
        
    public:
        future_error(boost::system::error_code ec)
        : logic_error(ec.message())
        , ec_(ec)
        {}
        
        boost::system::error_code const &code() const noexcept
        { return ec_; }
        
        const char *what() const noexcept
        { return code().message().c_str(); }
    };
    
    class future_uninitialized : public future_error {
    public:
        future_uninitialized()
        : future_error(boost::system::make_error_code(future_errc::no_state))
        {}
    };
    
    class future_already_retrieved : public future_error {
    public:
        future_already_retrieved()
        : future_error(boost::system::make_error_code(future_errc::future_already_retrieved))
        {}
    };
    
    class broken_promise : public future_error {
    public:
        broken_promise()
        : future_error(boost::system::make_error_code(future_errc::broken_promise))
        {}
    };
    
    class promise_already_satisfied : public future_error {
    public:
        promise_already_satisfied()
        : future_error(boost::system::make_error_code(future_errc::promise_already_satisfied))
        {}
    };
    
    class promise_uninitialized : public future_error {
    public:
        promise_uninitialized()
        : future_error(boost::system::make_error_code(future_errc::no_state))
        {}
    };
    
    class packaged_task_uninitialized : public future_error {
    public:
        packaged_task_uninitialized()
        : future_error(boost::system::make_error_code(future_errc::no_state))
        {}
    };
}}  // End of namespace boost::green_thread

#endif
