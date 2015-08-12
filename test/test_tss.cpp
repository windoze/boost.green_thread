#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>

#include <boost/green_thread.hpp>
#define BOOST_DONT_GREENIFY_STD_STREAM
#define BOOST_DONT_GREENIFY_MAIN
#include <boost/green_thread/greenify.hpp>

using namespace boost::green_thread;

void f(int x) {
    const int count=1000;
    static thread_specific_ptr<int> p;
    p.reset(new int());
    for(int i=0; i<count; i++) {
        this_thread::yield();
        *p += x;
    }
    BOOST_REQUIRE(*p==x*count);
}

BOOST_AUTO_TEST_CASE(test_fss) {
    greenify_with_sched(scheduler(), [](){
        get_scheduler().add_worker_thread(3);
        thread_group threads;
        for(int n=0; n<100; n++) {
            threads.create_thread(f, n);
        }
        threads.join_all();
    });
}
