//
//  use_future.hpp
//  Boost.GreenThread
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// Copyright (c) 2015 Chen Xu
//

#ifndef BOOST_GREEN_THREAD_ASIO_USE_FUTURE_HPP
#define BOOST_GREEN_THREAD_ASIO_USE_FUTURE_HPP

#include <memory>

#include <boost/config.hpp>
#include <boost/asio/detail/config.hpp>

namespace boost { namespace green_thread { namespace asio {
    /**
     * class use_future_t
     * 
     * Instance of the class can be used as completion handler for Boost.ASIO async_* call,
     * makes calls return a boost::green_thread::future.
     *
     * @see boost::green_thread::future
     */
    template<typename Allocator = std::allocator<void>>
    class use_future_t
    {
    public:
        typedef Allocator allocator_type;
        
        /// Construct using default-constructed allocator.
        BOOST_CONSTEXPR use_future_t()
        {}
        
        /// Construct using specified allocator.
        explicit use_future_t(Allocator const& allocator)
        : allocator_(allocator)
        {}
        
        /// Specify an alternate allocator.
        template<typename OtherAllocator>
        use_future_t<OtherAllocator> operator[](OtherAllocator const& allocator) const
        { return use_future_t<OtherAllocator>(allocator); }
        
        /// Obtain allocator.
        allocator_type get_allocator() const
        { return allocator_; }
        
    private:
        Allocator allocator_;
    };
    
    /// The predefined instance of use_future_t can be used directly.
    BOOST_CONSTEXPR_OR_CONST use_future_t<> use_future;
}}} // End of namespace boost::green_thread::asio

#include <boost/green_thread/asio/detail/use_future.hpp>

#endif
