//
//  greenify.hpp
//  Boost.GreenThread
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// Copyright (c) 2015 Chen Xu
//

#ifndef BOOST_GREEN_THREAD_GREENIFY_HPP
#define BOOST_GREEN_THREAD_GREENIFY_HPP

#include <type_traits>
#include <memory>
#include <boost/green_thread/thread.hpp>

/// Define this to stop using greenified std stream
//#define BOOST_DONT_GREENIFY_STD_STREAM
#include <boost/green_thread/detail/std_stream_guard.hpp>

/// Define this to stop using default main implementation
//#define BOOST_DONT_GREENIFY_MAIN

namespace boost { namespace green_thread {
    namespace detail {
        // HACK: C++ forbids variable with `void` type as it is always incomplete
        template<typename T>
        struct non_void {
            typedef T type;
            template<typename Fn, typename ...Args>
            static void run(type &t, Fn &&fn, Args&&... args)
            { t=std::forward<Fn>(fn)(std::forward<Args>(args)...); }
        };
        template<>
        struct non_void<void> {
            typedef int type;
            template<typename Fn, typename ...Args>
            static void run(type &t, Fn &&fn, Args&&... args)
            { t=0; std::forward<Fn>(fn)(std::forward<Args>(args)...); }
        };
    }   // End of namespace detail
    
    /**
     * Start the scheduler and create the first thread in it
     *
     * @param sched the scheduler to run the thread
     * @param fn the entry point of the thread
     * @param args the arguments for the thread
     */
    // NOTE: I cannot make this specialization work in VC2015, gave up and changed the name...
    template<typename Fn, typename ...Args>
    auto greenify_with_sched(scheduler &&sched, Fn &&fn, Args&& ...args)
    -> typename std::result_of<Fn(Args...)>::type
    {
        typedef typename std::result_of<Fn(Args...)>::type result_type;
        typedef detail::non_void<result_type> non_void_result;
        typename non_void_result::type ret{};
        try {
            sched.start();
            thread f(sched, [&](){
                detail::std_stream_guard guard;
                // Disable unused variable warning
                (void)guard;
                non_void_result::run(ret, std::forward<Fn>(fn), std::forward<Args>(args)...);
            });
            sched.join();
        } catch (std::exception& e) {
            std::cerr << "Exception: " << e.what() << "\n";
        }
        // No-op if `result_type` is `void`
        return result_type(ret);
    }
    
    /**
     * Start the scheduler and create the first thread in it
     *
     * @param sched the scheduler to run the thread
     * @param fn the entry point of the thread
     * @param args the arguments for the thread
     */
    // NOTE: I cannot make this specialization work in VC2015, gave up and changed the name...
    template<typename Fn, typename ...Args>
    auto greenify_with_sched(scheduler &sched, Fn &&fn, Args&& ...args)
    -> typename std::result_of<Fn(Args...)>::type
    {
        typedef typename std::result_of<Fn(Args...)>::type result_type;
        typedef detail::non_void<result_type> non_void_result;
        typename non_void_result::type ret{};
        try {
            sched.start();
            thread f(sched, [&](){
                detail::std_stream_guard guard;
                // Disable unused variable warning
                (void)guard;
                non_void_result::run(ret, std::forward<Fn>(fn), std::forward<Args>(args)...);
            });
            sched.join();
        } catch (std::exception& e) {
            std::cerr << "Exception: " << e.what() << "\n";
        }
        // No-op if `result_type` is `void`
        return result_type(ret);
    }

    /**
     * Start the default scheduler and create the first thread in it
     *
     * @param fn the entry point of the thread
     * @param args the arguments for the thread
     */
    template<typename Fn, typename ...Args>
    auto greenify(Fn &&fn, Args&& ...args)
    -> typename std::result_of<Fn(Args...)>::type
    {
        return greenify_with_sched(scheduler::get_instance(),
                                   std::forward<Fn>(fn),
                                   std::forward<Args>(args)...);
    }
}}  // End of namespace boost::green_thread

#if !defined(BOOST_DONT_GREENIFY_MAIN)
namespace boost { namespace green_thread {
    // Forward declaration of real entry point
    int main(int argc, char *argv[]);
}}  // End of namespace boost::green_thread
            
int main(int argc, char *argv[]) {
    return boost::green_thread::greenify(boost::green_thread::main, argc, argv);
}
#endif  // !defined(BOOST_DONT_GREENIFY_MAIN)

#endif
