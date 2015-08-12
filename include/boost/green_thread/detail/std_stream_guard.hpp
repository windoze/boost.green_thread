//
//  std_stream_guard.hpp
//  Boost.GreenThread
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// Copyright (c) 2015 Chen Xu
//

#ifndef BOOST_GREEN_THREAD_DETAIL_STD_STREAM_GUARD_HPP
#define BOOST_GREEN_THREAD_DETAIL_STD_STREAM_GUARD_HPP

#include <iostream>
#include <boost/asio/posix/stream_descriptor.hpp>
#include <boost/asio/windows/stream_handle.hpp>
#include <boost/green_thread/streambuf.hpp>

// Doesn't work under Windows
#if defined(_WIN32)
#   define BOOST_DONT_GREENIFY_STD_STREAM
#endif

namespace boost { namespace green_thread { namespace detail {
#if !defined(BOOST_DONT_GREENIFY_STD_STREAM)
        template<typename Stream, typename Desc, typename CharT=char, typename Traits=std::char_traits<CharT>>
        struct guard {
            typedef typename Desc::native_handle_type native_handle_type;
            typedef streambuf<Desc> sbuf_t;
            typedef std::unique_ptr<sbuf_t> sbuf_ptr_t;
            
            guard(Stream &s, native_handle_type h, bool unbuffered=false)
            : new_buf_(new sbuf_t(asio::get_io_service()))
            , stream_(s)
            {
                old_buf_=stream_.rdbuf(new_buf_.get());
                new_buf_->assign(h);
                // Don't sync with stdio, it doesn't work anyway
                stream_.sync_with_stdio(false);
                // Set to unbuffered if needed
                if(unbuffered) stream_.rdbuf()->pubsetbuf(0, 0);
            }
            
            ~guard() {
                flush(stream_);
#if BOOST_ASIO_HAS_POSIX_STREAM_DESCRIPTOR
                new_buf_->release();
#endif
                stream_.rdbuf(old_buf_);
            }
            
            // HACK: only ostream can be flushed
            void flush(std::basic_ostream<CharT, Traits> &s) { s.flush(); }
            void flush(std::basic_istream<CharT, Traits> &s) {}
            
            sbuf_ptr_t new_buf_;
            Stream &stream_;
            std::streambuf *old_buf_=0;
        };

#ifdef BOOST_ASIO_HAS_POSIX_STREAM_DESCRIPTOR
        // POSIX platform
        typedef boost::asio::posix::stream_descriptor std_handle_type;
        typedef std_handle_type::native_handle_type native_handle_type;
        native_handle_type std_in_handle() { return 0; }
        native_handle_type std_out_handle() { return 1; }
        native_handle_type std_err_handle() { return 2; }
#elif defined(BOOST_ASIO_HAS_WINDOWS_STREAM_HANDLE)
        // Windows platform
        typedef boost::asio::windows::stream_handle std_handle_type;
        typedef std_handle_type::native_handle_type native_handle_type;
        native_handle_type std_in_handle() { return ::GetStdHandle(STD_INPUT_HANDLE); }
        native_handle_type std_out_handle() { return ::GetStdHandle(STD_OUTPUT_HANDLE); }
        native_handle_type std_err_handle() { return ::GetStdHandle(STD_ERROR_HANDLE); }
#else
#   error("Unsupported platform")
#endif
        
        struct std_stream_guard {
            guard<std::istream, std_handle_type> cin_guard_{std::cin, std_in_handle()};
            guard<std::ostream, std_handle_type> cout_guard_{std::cout, std_out_handle()};
            guard<std::ostream, std_handle_type> cerr_guard_{std::cerr, std_err_handle(), true};
        };
        
#else   // !defined(BOOST_DONT_GREENIFY_STD_STREAM), No std stream guard, do nothing
        struct std_stream_guard {};
#endif  // !defined(BOOST_DONT_GREENIFY_STD_STREAM)
      
}}}	// End of namespace boost::green_thread::detail

#endif
