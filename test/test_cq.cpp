#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>

#include <boost/green_thread.hpp>
#define BOOST_DONT_GREENIFY_STD_STREAM
#define BOOST_DONT_GREENIFY_MAIN
#include <boost/green_thread/greenify.hpp>

using namespace boost::green_thread;
concurrent_queue<int> cq;

constexpr int children=1000;
constexpr size_t max_num=100;
constexpr long sum=max_num*(max_num+1)/2*children;
long result=0;

barrier bar(children);

void child() {
    for (int i=1; i<=max_num; i++) {
        cq.push(i);
    }
    // Queue is closed only if all child threads are finished
    if(bar.wait()) cq.close();
}

void parent() {
    for (int n=0; n<children; n++) {
        thread(child).detach();
    }
    int s=0;
    for (int popped : cq) {
        s+=popped;
    }
    result=s;
}

BOOST_AUTO_TEST_CASE(asio_with_yield) {
    greenify_with_sched(scheduler(), [](){
        get_scheduler().add_worker_thread(3);
        thread_group threads;
        threads.create_thread(parent);
        threads.join_all();
    });
    BOOST_REQUIRE(sum==result);
}
