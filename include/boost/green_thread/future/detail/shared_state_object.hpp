//
//  shared_state_object.hpp
//  Boost.GreenThread
//
// Copyright 2015 Chen Xu
// Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_GREEN_THREAD_FUTURE_DETAIL_SHARED_STATE_OBJECT_HPP
#define BOOST_GREEN_THREAD_FUTURE_DETAIL_SHARED_STATE_OBJECT_HPP

#include <boost/config.hpp>
#include <boost/green_thread/future/detail/shared_state.hpp>

namespace boost { namespace green_thread { namespace detail {
    template< typename R, typename Allocator >
    class shared_state_object : public shared_state< R >
    {
    public:
        typedef typename Allocator::template rebind<
        shared_state_object< R, Allocator >
        >::other                                      allocator_t;
        
        shared_state_object( allocator_t const& alloc) :
        shared_state< R >(), alloc_( alloc)
        {}
        
    protected:
        void deallocate_future()
        { destroy_( alloc_, this); }
        
    private:
        allocator_t             alloc_;
        
        static void destroy_( allocator_t & alloc, shared_state_object * p)
        {
            alloc.destroy( p);
            alloc.deallocate( p, 1);
        }
    };
}}} // End of namespace boost::green_thread::detail

#endif
