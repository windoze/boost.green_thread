#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>

#include <vector>
#include <boost/thread/thread.hpp>
#include <boost/chrono/system_clocks.hpp>
#include <boost/green_thread/thread.hpp>

// By defining this, Boost.GreenThread will not replace stream buffers for std streams,
// blocking of std streams will block a thread of scheduler.
// This is needed if you're using multiple schedulers, and more than one of them
// need to access std streams
#define BOOST_DONT_GREENIFY_STD_STREAM
#define BOOST_DONT_GREENIFY_MAIN
#include <boost/green_thread/greenify.hpp>

using namespace boost::green_thread;

boost::mutex cout_mtx;

/// A test data structure
struct data {
    /// default constructor sets member
    data(int x) : n(x) {}
    /// move constructor clears source
    data(data &&other) : n(other.n) { other.n=0; }
    /// copy constructor
    data(const data &other) : n(other.n) {}
    int n=0;
};

void f1(data d) {
    d.n=100;
}

void f2(data &d) {
    d.n=200;
}

struct thread_entry1 {
    void operator()(data d) const {
        d.n=300;
    }
};

struct thread_entry2 {
    void operator()(data &d) const {
        d.n=400;
    }
};

void ex() {
    // Throw something, has not to be std::exception
    throw std::string("exception from child thread");
}

void test_interrupted1() {
    try {
        this_thread::sleep_for(boost::chrono::seconds(1));
        // thread should be interrupted
        BOOST_REQUIRE(false);
    } catch(thread_interrupted) {
        // Interrupted
    }
}

void test_interrupted2() {
    try {
        this_thread::disable_interruption d1;
        this_thread::sleep_for(boost::chrono::seconds(1));
        // thread should not be interrupted
        BOOST_REQUIRE(true);
    } catch(thread_interrupted) {
        BOOST_REQUIRE(false);
    }
}

void test_interrupted3() {
    try {
        this_thread::disable_interruption d1;
        this_thread::restore_interruption r1;
        this_thread::sleep_for(boost::chrono::seconds(1));
        // thread should be interrupted
        BOOST_REQUIRE(false);
    } catch(thread_interrupted) {
        // Interrupted
        BOOST_REQUIRE(true);
    }
}

void test_interruptor(void (*t)()) {
    thread f(t);
    f.interrupt();
    f.join();
}

int main_thread(int n) {
    thread_group threads;
    
    data d1(1);
    data d2(2);
    data d3(3);
    data d4(4);
    data d5(5);
    data d6(6);
    
    // Plain function as the entry
    // Make a copy, d1 won't change
    threads.create_thread(f1, d1);
    // Move, d2.n becomes 0
    threads.create_thread(f1, std::move(d2));
    // Ref, d3.n will be changed
    threads.create_thread(f2, std::ref(d3));
    // Don't compile
    // threads.push_back(thread(f2, std::cref(d3)));
    
    // Functor as the entry
    // Make a copy d4 won't change
    threads.create_thread(thread_entry1(), d4);
    // Move, d5 becomes 0
    threads.create_thread(thread_entry1(), std::move(d5));
    // Ref, d6 will be changed
    threads.create_thread(thread_entry2(), std::ref(d6));
    
    // join with propagation
    thread fex1(ex);
    try {
        fex1.join(true);
        // joining will throw
        BOOST_REQUIRE(false);
    } catch(std::string &e) {
        BOOST_REQUIRE(e=="exception from child thread");
    }
    
    // join without propagation, will cause std::terminate() to be called
    /*
    thread fex2(ex);
    fex2.set_name("ex2");
    try {
        // joining will not throw
        fex2.join();
    } catch(...) {
        // No exception should be thrown
        BOOST_REQUIRE(false);
    }
     */
    
    // Test interruption
    threads.create_thread(test_interruptor, test_interrupted1);
    threads.create_thread(test_interruptor, test_interrupted2);
    threads.create_thread(test_interruptor, test_interrupted3);
    
    threads.join_all();
    
    // d1.n unchanged
    BOOST_REQUIRE(d1.n==1);
    // d2.n cleared
    BOOST_REQUIRE(d2.n==0);
    // d3.n changed
    BOOST_REQUIRE(d3.n=200);
    // d4.n unchanged
    BOOST_REQUIRE(d4.n==4);
    // d5.n cleared
    BOOST_REQUIRE(d5.n==0);
    // d6.n changed
    BOOST_REQUIRE(d6.n=400);
    // We need a lock as the std::cout is shared across multiple schedulers
    boost::lock_guard<boost::mutex> lk(cout_mtx);
    return 0;
}

BOOST_AUTO_TEST_CASE(test_threads) {
    // Create 10 schedulers from a thread belongs to the default scheduler
    std::vector<boost::thread> threads;
    for(size_t i=0; i<10; i++) {
        threads.emplace_back([i](){
            boost::green_thread::greenify_with_sched(boost::green_thread::scheduler(), main_thread, i);
        });
    }
    for(auto &t : threads) t.join();
}
