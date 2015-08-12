#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>

#include <boost/random.hpp>
#include <boost/chrono/system_clocks.hpp>
#define BOOST_DONT_GREENIFY_STD_STREAM
#define BOOST_DONT_GREENIFY_MAIN
#include <boost/green_thread/greenify.hpp>

using namespace boost::green_thread;
mutex m;
recursive_mutex m1;

void f(int n) {
    boost::random::mt19937 rng;
    boost::random::uniform_int_distribution<> three(1,3);
    for (int i=0; i<100; i++) {
        boost::unique_lock<mutex> lock(m);
        this_thread::sleep_for(boost::chrono::milliseconds(three(rng)));
        //printf("f(%d)\n", n);
    }
    //printf("f(%d) exiting\n", n);
}

void f1(int n) {
    boost::random::mt19937 rng;
    boost::random::uniform_int_distribution<> three(1,3);
    for (int i=0; i<10; i++) {
        boost::unique_lock<recursive_mutex> lock(m1);
        {
            boost::unique_lock<recursive_mutex> lock(m1);
            this_thread::sleep_for(boost::chrono::milliseconds(three(rng)));
            //printf("f1(%d)\n", n);
        }
        this_thread::sleep_for(boost::chrono::milliseconds(three(rng)));
    }
}

timed_mutex tm;

void child() {
    this_thread::yield();
    bool ret=tm.try_lock_for(boost::chrono::milliseconds(10));
    ret=tm.try_lock_until(boost::chrono::system_clock::now()+boost::chrono::seconds(2));
    BOOST_REQUIRE(ret);
    tm.unlock();
    //printf("child()\n");
}

void parent() {
    thread f(child);
    tm.lock();
    this_thread::sleep_for(boost::chrono::seconds(1));
    tm.unlock();
    //printf("parent():1\n");
    f.join();
    //printf("parent():2\n");
}

recursive_timed_mutex rtm;

void rchild() {
    this_thread::yield();
    bool ret=rtm.try_lock_for(boost::chrono::milliseconds(10));
    ret=rtm.try_lock_until(boost::chrono::steady_clock::now()+boost::chrono::seconds(2));
    BOOST_REQUIRE(ret);
    rtm.unlock();
    //printf("child()\n");
}

void rparent() {
    bool ret=rtm.try_lock();
    ret=rtm.try_lock();
    BOOST_REQUIRE(ret);
    rtm.unlock();
    rtm.unlock();
    thread f(rchild);
    rtm.lock();
    this_thread::sleep_for(boost::chrono::seconds(1));
    rtm.unlock();
    //printf("parent():1\n");
    f.join();
    //printf("parent():2\n");
}

BOOST_AUTO_TEST_CASE(test_mutex) {
    greenify_with_sched(scheduler(), [](){
        get_scheduler().add_worker_thread(3);

        thread_group threads;
        threads.create_thread(parent);
        threads.create_thread(rparent);
        for (int i=0; i<10; i++) {
            threads.create_thread(f, i);
        }
        for (int i=0; i<10; i++) {
            threads.create_thread(f1, i);
        }
        threads.join_all();
    });
}
