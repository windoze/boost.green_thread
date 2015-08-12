//
//  thread_object.cpp
//  Boost.GreenThread
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// Copyright (c) 2015 Chen Xu
//

#if defined(_WIN32)
//  <winsock2.h> must be included before <windows.h>
#   include <winsock2.h>
#endif

#include <memory>
#include <algorithm>
#if defined(_WIN32)
#   include <boost/coroutine/standard_stack_allocator.hpp>
#else
#   include <boost/coroutine/protected_stack_allocator.hpp>
#endif
#include <boost/coroutine/stack_context.hpp>

#include <boost/green_thread/thread_only.hpp>
#include <boost/green_thread/tss.hpp>
#include <boost/green_thread/mutex.hpp>
#include <boost/green_thread/condition_variable.hpp>

#include "thread_object.hpp"
#include "scheduler_object.hpp"

static const auto NOT_A_THREAD=boost::green_thread::thread_exception(boost::system::errc::no_such_process);
static const auto DEADLOCK=boost::green_thread::thread_exception(boost::system::errc::resource_deadlock_would_occur);

namespace boost { namespace green_thread { namespace detail {
#ifdef BOOST_USE_SEGMENTED_STACKS
#   define BOOST_COROUTINE_STACK_ALLOCATOR boost::coroutines::basic_segmented_stack_allocator
#else
#   if defined(_WIN32)
#       define BOOST_COROUTINE_STACK_ALLOCATOR boost::coroutines::basic_standard_stack_allocator
#   else
#       define BOOST_COROUTINE_STACK_ALLOCATOR boost::coroutines::basic_protected_stack_allocator
#   endif
#endif

    typedef BOOST_COROUTINE_STACK_ALLOCATOR<boost::coroutines::stack_traits> stack_allocator;
    
    thread_object::thread_object(scheduler_ptr_t sched, thread_data_base *entry)
    : sched_(sched)
    , thread_strand_(std::make_shared<boost::asio::strand>(sched_->io_service_))
    , state_(READY)
    , entry_(entry)
    , runner_(std::bind(&thread_object::runner_wrapper, this, std::placeholders::_1),
              boost::coroutines::attributes(),
              stack_allocator() )
    , caller_(0)
    {}
    
    thread_object::thread_object(scheduler_ptr_t sched, strand_ptr_t strand, thread_data_base *entry)
    : sched_(sched)
    , thread_strand_(strand)
    , state_(READY)
    , entry_(entry)
    , runner_(std::bind(&thread_object::runner_wrapper, this, std::placeholders::_1),
              boost::coroutines::attributes(),
              stack_allocator() )
    , caller_(0)
    {}
    
    thread_object::~thread_object() {
        if (state_!=STOPPED) {
            // std::thread will call std::terminate if deleting a unstopped thread
            std::terminate();
        }
        if (uncaught_exception_) {
            // There is an uncaught exception not propagated to joiner
            std::terminate();
        }
    }
    
    void thread_object::set_name(const std::string &s) {
        boost::lock_guard<spinlock> lock(mtx_);
        name_=s;
    }
    
    std::string thread_object::get_name() {
        boost::lock_guard<spinlock> lock(mtx_);
        return name_;
    }
    
    void thread_object::runner_wrapper(caller_t &c) {
        struct cleaner {
            cleaner(spinlock &mtx,
                    cleanup_queue_t &q,
                    fss_map_t &fss)
            : mtx_(mtx)
            , q_(q)
            , fss_(fss)
            {}
            
            ~cleaner()
            try {
                // No exception should be thrown in destructor
                cleanup_queue_t temp;
                {
                    // Move cleanup queue content out
                    boost::lock_guard<spinlock> lock(mtx_);
                    temp.swap(q_);
                }
                for (std::function<void()> f: temp) {
                    f();
                }
                for (auto &v: fss_) {
                    if (v.second.first && v.second.second) {
                        (*(v.second.first))(v.second.second);
                    }
                }
            } catch(...) {
                // TODO: Error
            }
            spinlock &mtx_;
            cleanup_queue_t &q_;
            fss_map_t &fss_;
        };
        // Need this to complete constructor without running entry_
        c(READY);
        
        // Now we're out of constructor
        caller_=&c;
        try {
            cleaner c(mtx_, cleanup_queue_, fss_);
            entry_->run();
        } catch(const boost::coroutines::detail::forced_unwind&) {
            // Boost.Coroutine requirement
            throw;
        } catch(...) {
            uncaught_exception_=std::current_exception();
        }
        // Clean thread arguments before thread destroy
        entry_.reset();
        // Thread function exits, set state to STOPPED
        c(STOPPED);
        caller_=0;
    }
    
    void thread_object::detach() {
        boost::lock_guard<spinlock> lock(mtx_);
        if (state_!=STOPPED) {
            // Hold a reference to this, make sure detached thread live till ends
            this_ref_=shared_from_this();
        }
    }
    
    void thread_object::one_step() {
        struct tls_guard {
            tls_guard(thread_object *pthis) {
                thread_object::get_current_thread_object()=pthis;
            }
            
            ~tls_guard() {
                thread_object::get_current_thread_object()=0;
            }
        };
        if (state_==READY) {
            state_=RUNNING;
        }
        // Keep running if necessary
        while (state_==RUNNING) {
            tls_guard guard(this);
            state_=runner_().get();
        }
        state_t s= state_;
        if (s==READY) {
            // Post this thread to the scheduler
            resume();
        } else if (s==BLOCKED) {
            // Must make sure this thread will be posted elsewhere later, otherwise it will hold forever
        } else if (s==STOPPED) {
            cleanup_queue_t temp;
            {
                // Move joining queue content out
                boost::lock_guard<spinlock> lock(mtx_);
                temp.swap(join_queue_);
            }
            // thread ended, clean up joining queue
            for (std::function<void()> f: temp) {
                f();
            }
            // Post exit message to scheduler
            get_thread_strand().post(std::bind(&scheduler_object::on_thread_exit, sched_, shared_from_this()));
        }
    }
    
    boost::asio::strand &thread_object::get_thread_strand() {
        return *thread_strand_;
    }
    
    // Switch out of thread context
    void thread_object::pause() {
        // Pre-condition
        // Can only pause current running thread
        assert(get_current_thread_object()==this);
        
        set_state(BLOCKED);
        
        // Check interruption when resumed
        boost::lock_guard<spinlock> lock(mtx_);
        if (interrupt_disable_level_==0 && interrupt_requested_) {
            BOOST_THROW_EXCEPTION(thread_interrupted());
        }
    }
    
    inline void activate_thread(thread_ptr_t this_thread) {
        // Pre-condition
        // Cannot activate current running thread
        assert(thread_object::get_current_thread_object()!=this_thread.get());
        
        this_thread->state_=thread_object::READY;
        this_thread->one_step();
    }
    
    void thread_object::activate() {
        if (thread_object::get_current_thread_object()
            && thread_object::get_current_thread_object()->sched_
            && (thread_object::get_current_thread_object()->sched_==sched_))
        {
            get_thread_strand().dispatch(std::bind(activate_thread, shared_from_this()));
        } else {
            resume();
        }
    }
    
    void thread_object::resume() {
        get_thread_strand().post(std::bind(activate_thread, shared_from_this()));
    }
    
    // Following functions can only be called inside coroutine
    void thread_object::yield(thread_ptr_t hint) {
        // Pre-condition
        // Can only pause current running thread
        assert(get_current_thread_object()==this);
        assert(state_==RUNNING);

        // Do yeild when:
        //  1. there is only 1 thread in this scheduler
        //  2. or, too many threads out there (thread_count > thread_count*2)
        //  3. or, hint is a thread that shares the strand with this one
        //  4. or, there is no hint (force yield)
        if ((sched_->threads_.size()==1)
            || (sched_->thread_count_>sched_->threads_.size()*2)
            || (hint && (hint->thread_strand_==thread_strand_))
            || !hint
            )
        {
            set_state(READY);
        }
    }

    void thread_object::join(thread_ptr_t f) {
        CHECK_CALLER(this);
        boost::lock_guard<spinlock> lock(f->mtx_);
        if (this==f.get()) {
            // The thread is joining itself
            BOOST_THROW_EXCEPTION(DEADLOCK);
        } else if (f->state_==STOPPED) {
            // f is already stopped, do nothing
            return;
        } else {
            f->join_queue_.push_back(std::bind(&thread_object::activate, shared_from_this()));
        }

        { relock_guard<spinlock> relock(f->mtx_); pause(); }
    }
    
    void propagate_exception(thread_ptr_t f) {
        std::exception_ptr e;
        std::swap(e, f->uncaught_exception_);
        // throw exception
        if (e) {
            std::rethrow_exception(e);
        }
    }
    
    void thread_object::join_and_rethrow(thread_ptr_t f) {
        CHECK_CALLER(this);
        boost::lock_guard<spinlock> lock(f->mtx_);
        if (this==f.get()) {
            // The thread is joining itself
            BOOST_THROW_EXCEPTION(DEADLOCK);
        } else if (f->state_==STOPPED) {
            // f is already stopped
            propagate_exception(f);
            return;
        } else {
            // std::cout << "thread(pthis) blocked" << std::endl;
            f->join_queue_.push_back(std::bind(&thread_object::activate, shared_from_this()));
        }

        { relock_guard<spinlock> relock(f->mtx_); pause(); }

        // Joining completed, propagate exception from joinee
        propagate_exception(f);
    }

    void thread_object::sleep_rel(duration_t d) {
        // Shortcut
        if (d==duration_t::zero()) {
            return;
        }
        CHECK_CALLER(this);
        timer_t sleep_timer(get_io_service());
        sleep_timer.expires_from_now(d);
        sleep_timer.async_wait(std::bind(&thread_object::activate, shared_from_this()));

        pause();
    }
    
    void thread_object::add_cleanup_function(std::function<void()> &&f) {
        boost::lock_guard<spinlock> lock(mtx_);
        cleanup_queue_.push_back(std::move(f));
    }
    
    void thread_object::interrupt() {
        boost::lock_guard<spinlock> lock(mtx_);
        if (interrupt_disable_level_==0) {
            interrupt_requested_=true;
        }
    }
    
    void set_tss_data(void const* key,
                      std::shared_ptr<tss_cleanup_function> func,
                      void* fss_data,
                      bool cleanup_existing)
    {
        if (thread_object::get_current_thread_object()) {
            if (!func && !fss_data) {
                // Remove fss if both func and data are NULL
                fss_map_t::iterator i=thread_object::get_current_thread_object()->fss_.find(key);
                if (i!=thread_object::get_current_thread_object()->fss_.end() ) {
                    // Clean up existing if it has a cleanup func
                    if(i->second.first)
                        (*(i->second.first.get()))(i->second.second);
                    thread_object::get_current_thread_object()->fss_.erase(i);
                }
            } else {
                // Clean existing if needed
                if (cleanup_existing) {
                    fss_map_t::iterator i=thread_object::get_current_thread_object()->fss_.find(key);
                    if (i!=thread_object::get_current_thread_object()->fss_.end() && (i->second.first)) {
                        // Clean up existing if it has a cleanup func
                        (*(i->second.first.get()))(i->second.second);
                    }
                }
                // Insert/update the key
                thread_object::get_current_thread_object()->fss_[key]={func, fss_data};
            }
        }
    }
    
    void* get_tss_data(void const* key) {
        if (thread_object::get_current_thread_object()) {
            fss_map_t::iterator i=thread_object::get_current_thread_object()->fss_.find(key);
            if (i!=thread_object::get_current_thread_object()->fss_.end()) {
                return i->second.second;
            } else {
                // Create if not exist
                //thread_object::current_thread_->fss_.insert({key, {std::shared_ptr<fss_cleanup_function>(), 0}});
            }
        }
        return 0;
    }
    
    thread_base::ptr_t get_current_thread_ptr() {
        if(!thread_object::get_current_thread_object()) {
            // Not a thread
            BOOST_THROW_EXCEPTION(NOT_A_THREAD);
        }
        return std::static_pointer_cast<thread_base>(thread_object::get_current_thread_object()->shared_from_this());
    }
}}}   // End of namespace boost::green_thread::detail

namespace boost { namespace green_thread {
    void thread::start() {
        if (auto cf=current_thread_object()) {
            // use current scheduler if we're in a thread
            impl_=cf->sched_->make_thread(data_.release());
        } else {
            // use default scheduler if we're not in a thread
            impl_=scheduler::get_instance().impl_->make_thread(data_.release());
        }
    }
    
    void thread::start(attributes attr) {
        if (auto cf=current_thread_object()) {
            // use current scheduler if we're in a thread
            switch(attr.policy) {
                case attributes::scheduling_policy::normal: {
                    // Create an isolated thread
                    impl_=cf->sched_->make_thread(data_.release());
                    break;
                }
                case attributes::scheduling_policy::stick_with_parent: {
                    // Create a thread shares strand with parent
                    impl_=cf->sched_->make_thread(current_thread_object()->thread_strand_, data_.release());
                    break;
                }
                default:
                    break;
            }
        } else {
            // use default scheduler if we're not in a thread
            impl_=scheduler::get_instance().impl_->make_thread(data_.release());
        }
    }
    
    void thread::start(scheduler &sched) {
        // use supplied scheduler to start thread
        impl_=sched.impl_->make_thread(data_.release());
    }
    
    thread::thread(thread &&other) noexcept
    : data_(std::move(other.data_))
    , impl_(std::move(other.impl_))
    {}
    
    thread& thread::operator=(thread &&other) noexcept {
        if (impl_) {
            // This thread is still active, std::thread will call std::terminate in the case
            std::terminate();
        }
        data_=std::move(other.data_);
        impl_=std::move(other.impl_);
        return *this;
    }
    
    void thread::set_name(const std::string &s) {
        impl_->set_name(s);
    }
    
    std::string thread::get_name() {
        return impl_->get_name();
    }
    
    bool thread::joinable() const noexcept {
        // Return true iff this is a thread and not the current calling thread
        // and 2 threads are in the same scheduler
        return (impl_ && current_thread_object()!=impl_.get())
        && (impl_->sched_==current_thread_object()->sched_);
    }
    
    thread::id thread::get_id() const noexcept {
        return reinterpret_cast<thread::id>(impl_.get());
    }
    
    void thread::join(bool propagate_exception) {
        if (!impl_) {
            BOOST_THROW_EXCEPTION(NOT_A_THREAD);
        }
        if (impl_.get()==current_thread_object()) {
            BOOST_THROW_EXCEPTION(DEADLOCK);
        }
        if (!joinable()) {
            BOOST_THROW_EXCEPTION(invalid_argument());
        }
        if (current_thread_object()) {
            if (propagate_exception) {
                current_thread_object()->join_and_rethrow(impl_);
            } else {
                current_thread_object()->join(impl_);
            }
        }
    }
    
    void thread::detach() {
        if (!(impl_ && current_thread_object()!=impl_.get())) {
            BOOST_THROW_EXCEPTION(NOT_A_THREAD);
        }
        detail::thread_ptr_t this_thread=impl_;
        impl_->get_thread_strand().post(std::bind(&detail::thread_object::detach, impl_));
        impl_.reset();
    }
    
    void thread::swap(thread &other) noexcept(true) {
        std::swap(impl_, other.impl_);
    }
    
    unsigned thread::hardware_concurrency() noexcept {
        return boost::thread::hardware_concurrency();
    }
    
    void thread::interrupt() {
        if (!impl_) {
            BOOST_THROW_EXCEPTION(NOT_A_THREAD);
        }
        boost::lock_guard<detail::spinlock> lock(impl_->mtx_);
        if (impl_->interrupt_disable_level_==0) {
            impl_->interrupt_requested_=true;
        }
    }
    
    namespace this_thread {
        void yield() {
            if (auto cf=current_thread_object()) {
                cf->yield();
            } else {
                BOOST_THROW_EXCEPTION(NOT_A_THREAD);
            }
        }
        
        thread::id get_id() {
            return reinterpret_cast<thread::id>(current_thread_object());
        }
        
        bool is_a_thread() noexcept(true) {
            return current_thread_object();
        }
        
        namespace detail {
            void sleep_rel(green_thread::detail::duration_t d) {
                if (auto cf=current_thread_object()) {
                    if (d<green_thread::detail::duration_t::zero()) {
                        BOOST_THROW_EXCEPTION(invalid_argument());
                    }
                    cf->sleep_rel(d);
                } else {
                    BOOST_THROW_EXCEPTION(NOT_A_THREAD);
                }
            }
            
            boost::asio::io_service &get_io_service() {
                if (auto cf=current_thread_object()) {
                    return cf->get_io_service();
                }
                BOOST_THROW_EXCEPTION(NOT_A_THREAD);
            }
            
            boost::asio::strand &get_strand() {
                if (auto cf=current_thread_object()) {
                    return cf->get_thread_strand();
                } else {
                    BOOST_THROW_EXCEPTION(NOT_A_THREAD);
                }
            }
        }   // End of namespace detail
        
        std::string get_name() {
            if (auto cf=current_thread_object()) {
                return cf->get_name();
            } else {
                BOOST_THROW_EXCEPTION(NOT_A_THREAD);
            }
        }
        
        void set_name(const std::string &name) {
            if (auto cf=current_thread_object()) {
                cf->set_name(name);
            } else {
                BOOST_THROW_EXCEPTION(NOT_A_THREAD);
            }
        }
        
        disable_interruption::disable_interruption() {
            if (auto cf=current_thread_object()) {
                boost::lock_guard<green_thread::detail::spinlock> lock(cf->mtx_);
                cf->interrupt_disable_level_++;
            } else {
                BOOST_THROW_EXCEPTION(NOT_A_THREAD);
            }
        }
        
        disable_interruption::~disable_interruption() {
            if (auto cf=current_thread_object()) {
                boost::lock_guard<green_thread::detail::spinlock> lock(cf->mtx_);
                cf->interrupt_disable_level_--;
            }
        }
        
        restore_interruption::restore_interruption() {
            if (auto cf=current_thread_object()) {
                boost::lock_guard<green_thread::detail::spinlock> lock(cf->mtx_);
                std::swap(cf->interrupt_disable_level_, level_);
            } else {
                BOOST_THROW_EXCEPTION(NOT_A_THREAD);
            }
        }
        
        restore_interruption::~restore_interruption() {
            if (auto cf=current_thread_object()) {
                boost::lock_guard<green_thread::detail::spinlock> lock(cf->mtx_);
                std::swap(cf->interrupt_disable_level_, level_);
            }
        }
        
        bool interruption_enabled() noexcept {
            if (auto cf=current_thread_object()) {
                boost::lock_guard<green_thread::detail::spinlock> lock(cf->mtx_);
                return cf->interrupt_disable_level_==0;
            }
            return false;
        }
        
        bool interruption_requested() noexcept {
            if (auto cf=current_thread_object()) {
                boost::lock_guard<green_thread::detail::spinlock> lock(cf->mtx_);
                return cf->interrupt_requested_==0;
            }
            return false;
        }
        
        void interruption_point() {
            if (auto cf=current_thread_object()) {
                boost::lock_guard<green_thread::detail::spinlock> lock(cf->mtx_);
                if (cf->interrupt_requested_) {
                    BOOST_THROW_EXCEPTION(green_thread::thread_interrupted());
                }
            }
        }
        
        void at_thread_exit(std::function<void()> &&f) {
            if (auto cf=current_thread_object()) {
                boost::lock_guard<green_thread::detail::spinlock> lock(cf->mtx_);
                cf->cleanup_queue_.push_back(std::forward<std::function<void()>>(f));
            }
        }
    }   // End of namespace this_thread
    
    scheduler get_scheduler() {
        if (auto cf=current_thread_object()) {
            return scheduler(cf->sched_);
        } else {
            BOOST_THROW_EXCEPTION(NOT_A_THREAD);
        }
    }
}}  // End of namespace boost::green_thread

