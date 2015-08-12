//
//  thread_group.hpp
//  Boost.GreenThread
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// (C) Copyright 2007-9 Anthony Williams
// (C) Copyright 2015 Chen Xu
//

#ifndef BOOST_GREEN_THREAD_THREAD_GROUP_HPP
#define BOOST_GREEN_THREAD_THREAD_GROUP_HPP

#include <list>
#include <algorithm>
#include <boost/thread/lock_guard.hpp>
#include <boost/green_thread/thread_only.hpp>
#include <boost/green_thread/shared_mutex.hpp>

namespace boost { namespace green_thread {
    /// thread_group
    class thread_group {
    private:
        /// thread_group is non-copyable
        thread_group(thread_group const&)=delete;
        thread_group& operator=(thread_group const&)=delete;

    public:
        /// constructor
        thread_group() {}
        
        /// destructor
        ~thread_group() {
            join_all();
            for(std::list<thread*>::iterator it=threads_.begin(), end=threads_.end(); it!=end; ++it) {
                delete *it;
            }
        }
        
        /**
         * check if the current thread is in the thread group
         */
        bool is_this_thread_in() {
            thread::id id = this_thread::get_id();
            shared_lock<shared_timed_mutex> guard(m_);
            for(std::list<thread*>::iterator it=threads_.begin(),end=threads_.end(); it!=end; ++it) {
                if ((*it)->get_id() == id)
                    return true;
            }
            return false;
        }
        
        /**
         * check if the given thread is in the thread group
         */
        bool is_thread_in(thread* thrd) {
            if(thrd) {
                thread::id id = thrd->get_id();
                shared_lock<shared_timed_mutex> guard(m_);
                for(std::list<thread*>::iterator it=threads_.begin(),end=threads_.end(); it!=end; ++it) {
                    if ((*it)->get_id() == id)
                        return true;
                }
                return false;
            } else {
                return false;
            }
        }
        
        /**
         * create a new thread in the thread group
         */
        template<typename Fn, typename... Args>
        thread* create_thread(Fn &&fn, Args&&... args)
        {
            boost::lock_guard<shared_timed_mutex> guard(m_);
            std::unique_ptr<thread> new_thread(new thread(std::forward<Fn>(fn), std::forward<Args>(args)...));
            threads_.push_back(new_thread.get());
            return new_thread.release();
        }
        
        /**
         * add an existing thread into the thread group
         */
        void add_thread(thread* thrd) {
            if(thrd) {
                boost::lock_guard<shared_timed_mutex> guard(m_);
                threads_.push_back(thrd);
            }
        }
        
        /**
         * remove a thread from the thread group
         */
        void remove_thread(thread* thrd) {
            boost::lock_guard<shared_timed_mutex> guard(m_);
            std::list<thread*>::iterator const it=std::find(threads_.begin(),threads_.end(),thrd);
            if(it!=threads_.end()) {
                threads_.erase(it);
            }
        }
        
        /**
         * wait until all threads exit
         */
        void join_all() {
            shared_lock<shared_timed_mutex> guard(m_);
            
            for(std::list<thread*>::iterator it=threads_.begin(),end=threads_.end(); it!=end; ++it) {
                if ((*it)->joinable())
                    (*it)->join();
            }
        }
        
        /**
         * returns the number of threads in the group
         */
        size_t size() const {
            shared_lock<shared_timed_mutex> guard(m_);
            return threads_.size();
        }
        
    private:
        std::list<thread*> threads_;
        mutable shared_timed_mutex m_;
    };
}}  // End of namespace boost::green_thread

#endif
