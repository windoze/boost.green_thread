//
//  thread_data.hpp
//  Boost.GreenThread
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// Copyright (c) 2015 Chen Xu
//

#ifndef BOOST_GREEN_THREAD_THREAD_DATA_HPP
#define BOOST_GREEN_THREAD_THREAD_DATA_HPP

#include <memory>
#include <boost/static_assert.hpp>
#include <boost/green_thread/detail/utility.hpp>

namespace boost { namespace green_thread { namespace detail {
    /// thread_data_base
    struct thread_data_base
    {
        virtual ~thread_data_base(){}
        virtual void run()=0;
    };
    
    /// thread_data
    template<typename F, class... ArgTypes>
    class thread_data : public thread_data_base
    {
    public:
        thread_data(F&& f_, ArgTypes&&... args_)
        : fp(std::forward<F>(f_), std::forward<ArgTypes>(args_)...)
        {}

        template <std::size_t... Indices>
        void run2(utility::tuple_indices<Indices...>)
        { utility::invoke(std::move(std::get<0>(fp)), std::move(std::get<Indices>(fp))...); }

        void run()
        {
            typedef typename utility::make_tuple_indices<std::tuple_size<std::tuple<F, ArgTypes...> >::value, 1>::type index_type;
            run2(index_type());
        }
        
    private:
        /// Non-copyable
        thread_data(const thread_data&)=delete;
        void operator=(const thread_data&)=delete;
        std::tuple<typename std::decay<F>::type, typename std::decay<ArgTypes>::type...> fp;
    };
    
    /**
     * make_thread_data
     *
     * wrap entry function and arguments into a tuple
     */
    template<typename F, class... ArgTypes>
    inline thread_data_base *make_thread_data(F&& f, ArgTypes&&... args)
    {
        return new thread_data<typename std::remove_reference<F>::type, ArgTypes...>(std::forward<F>(f),
                                                                                    std::forward<ArgTypes>(args)...);
    }
}}} // End of namespace boost::green_thread::detail
#endif
