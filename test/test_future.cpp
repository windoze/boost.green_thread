#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>

#include <boost/lexical_cast.hpp>
#include <boost/chrono/system_clocks.hpp>
#include <boost/green_thread.hpp>
#define BOOST_DONT_GREENIFY_STD_STREAM
#define BOOST_DONT_GREENIFY_MAIN
#include <boost/green_thread/greenify.hpp>

using namespace boost::green_thread;

int f1(int x) {
    this_thread::sleep_for(boost::chrono::milliseconds(100));
    return x*2;
}

struct f2 {
    f2(int n) : n_(n) {}
    int operator()(int x, int y) const {
        this_thread::sleep_for(boost::chrono::milliseconds(30));
        return x+y+n_;
    }
    int n_;
};

BOOST_AUTO_TEST_CASE(test_future) {
    int n1=0;
    int n2=1;
    int n3=2;
    greenify_with_sched(scheduler(), [&](){
        // future from a packaged_task
        packaged_task<int()> task([](){ return 7; }); // wrap the function
        future<int> f1 = task.get_future();  // get a future
        thread(std::move(task)).detach(); // launch on a thread

        // future from an async()
        future<int> f2 = async([](){ return 8; });

        // future from a promise
        promise<int> p;
        future<int> f3 = p.get_future();
        thread([](promise<int> p){
            p.set_value(9);
        }, std::move(p)).detach();

        f1.wait();
        f2.wait();
        f3.wait();
        n1=f1.get();
        n2=f2.get();
        n3=f3.get();
    });
    BOOST_REQUIRE(n1==7);
    BOOST_REQUIRE(n2==8);
    BOOST_REQUIRE(n3==9);
}

BOOST_AUTO_TEST_CASE(test_async) {
    int n1=0;
    int n2=1;
    greenify_with_sched(scheduler(), [&](){
        n1=async(f2(100), async(f1, 42).get(), async(f1, 24).get()).get();
        n2=f2(100)(f1(42), f1(24));
    });
    BOOST_REQUIRE(n1==n2);
}

BOOST_AUTO_TEST_CASE(test_async_executor) {
    int n1=0;
    int n2=1;
    greenify_with_sched(scheduler(), [&](){
        async_executor<int> ex(10);
        n1=ex(f2(100), ex(f1, 42).get(), ex(f1, 24).get()).get();
        n2=f2(100)(f1(42), f1(24));
    });
    BOOST_REQUIRE(n1==n2);
}

BOOST_AUTO_TEST_CASE(test_async_function) {
    int n1=0;
    int n2=1;
    greenify_with_sched(scheduler(), [&](){
        auto af1=make_async(f1);
        auto af2=make_async(f2(100));
        n1=af2(af1(42).get(), af1(24).get()).get();
        n2=f2(100)(f1(42), f1(24));
    });
    BOOST_REQUIRE(n1==n2);
}

BOOST_AUTO_TEST_CASE(test_wait_for_any1) {
    int n=0;
    greenify_with_sched(scheduler(), [&](){
        future<void> f0=async([](){
            this_thread::sleep_for(boost::chrono::seconds(1));
        });
        future<int> f1=async([](){
            this_thread::sleep_for(boost::chrono::milliseconds(100));
            return 100;
        });
        future<double> f2=async([](){
            this_thread::sleep_for(boost::chrono::milliseconds(300));
            return 100.5;
        });
        n=wait_for_any(f0, f1, f2);
        // 2nd future should be ready
    });
    BOOST_REQUIRE(n==1);
}

BOOST_AUTO_TEST_CASE(test_wait_for_any2) {
    std::vector<future<void>> fv;
    std::vector<future<void>>::const_iterator i=fv.end();
    greenify_with_sched(scheduler(), [&](){
        for (size_t i=0; i<10; i++) {
            fv.push_back(async([i](){
                this_thread::sleep_for(boost::chrono::milliseconds(100*(i+1)));
            }));
        }
        i=wait_for_any(fv.cbegin(), fv.cend());
        // 1st future should be ready
    });
    BOOST_REQUIRE(i==fv.begin());
}

BOOST_AUTO_TEST_CASE(test_wait_for_any3) {
    size_t n=0;
    greenify_with_sched(scheduler(), [&](){
        future<void> f0=async([](){
            this_thread::sleep_for(boost::chrono::seconds(1));
        });
        shared_future<double> f1=async([](){
            this_thread::sleep_for(boost::chrono::milliseconds(300));
            return 100.5;
        }).share();
        future<int> f2=make_ready_future(100);
        n=wait_for_any(std::tie(f0, f1, f2));
        // 3rd future should be ready
    });
    BOOST_REQUIRE(n==2);
}

BOOST_AUTO_TEST_CASE(test_wait_for_all1) {
    boost::chrono::system_clock::duration dur=boost::chrono::seconds(0);
    greenify_with_sched(scheduler(), [&](){
        auto start=boost::chrono::system_clock::now();
        future<void> f0=async([](){
            this_thread::sleep_for(boost::chrono::seconds(1));
        });
        future<int> f1=async([](){
            this_thread::sleep_for(boost::chrono::milliseconds(100));
            return 100;
        });
        future<double> f2=async([](){
            this_thread::sleep_for(boost::chrono::milliseconds(300));
            return 100.5;
        });
        wait_for_all(f0, f1, f2);
        auto stop=boost::chrono::system_clock::now();
        dur=stop-start;
    });
    BOOST_REQUIRE(dur>=boost::chrono::seconds(1));
}

BOOST_AUTO_TEST_CASE(test_wait_for_all2) {
    constexpr size_t c=10;
    boost::chrono::system_clock::duration dur=boost::chrono::seconds(0);
    greenify_with_sched(scheduler(), [&](){
        auto start=boost::chrono::system_clock::now();
        std::vector<future<void>> fv;
        for (size_t i=0; i<c; i++) {
            fv.push_back(async([i](){
                this_thread::sleep_for(boost::chrono::milliseconds(100*(i+1)));
            }));
        }
        wait_for_all(fv.cbegin(), fv.cend());
        auto stop=boost::chrono::system_clock::now();
        dur=stop-start;
    });
    BOOST_REQUIRE(dur>=boost::chrono::milliseconds(100*c));
}

BOOST_AUTO_TEST_CASE(test_wait_for_all3) {
    boost::chrono::system_clock::duration dur=boost::chrono::seconds(0);
    greenify_with_sched(scheduler(), [&](){
        auto start=boost::chrono::system_clock::now();
        auto f0=async([](){
            this_thread::sleep_for(boost::chrono::seconds(1));
        });
        auto f1=async([](){
            this_thread::sleep_for(boost::chrono::milliseconds(100));
            return 100;
        });
        auto f2=async([](){
            this_thread::sleep_for(boost::chrono::milliseconds(300));
            return 100.5;
        });
        wait_for_all(std::tie(f0, f1, f2));
        auto stop=boost::chrono::system_clock::now();
        dur=stop-start;
    });
    BOOST_REQUIRE(dur>=boost::chrono::seconds(1));
}

BOOST_AUTO_TEST_CASE(test_async_wait_for_any1) {
    std::vector<future<void>> fv;
    std::vector<future<void>>::const_iterator i=fv.end();
    greenify_with_sched(scheduler(), [&](){
        for (size_t i=0; i<10; i++) {
            fv.push_back(async([i](){
                this_thread::sleep_for(boost::chrono::milliseconds(100*(i+1)));
            }));
        }
        auto f=async_wait_for_any(fv.cbegin(), fv.cend());
        i=f.get();
        // 1st future should be ready
    });
    BOOST_REQUIRE(i==fv.begin());
}

BOOST_AUTO_TEST_CASE(test_async_wait_for_any2) {
    std::vector<future<void>> fv1;
    std::vector<future<void>>::const_iterator i1=fv1.end();
    std::vector<future<int>> fv2;
    std::vector<future<int>>::const_iterator i2=fv2.end();
    greenify_with_sched(scheduler(), [&](){
        for (int i=0; i<10; i++) {
            fv1.push_back(async([i](){
                this_thread::sleep_for(boost::chrono::milliseconds(100*(i+1)));
            }));
        }
        auto f1=async_wait_for_any(fv1.cbegin(), fv1.cend());

        for (int i=0; i<10; i++) {
            fv2.push_back(async([i](){
                this_thread::sleep_for(boost::chrono::milliseconds(90*(i+1)));
                return i;
            }));
        }
        auto f2=async_wait_for_any(fv2.cbegin(), fv2.cend());
        wait_for_all(f1, f2);
        i1=f1.get();
        i2=f2.get();
    });
    // 1st future should be ready
    BOOST_REQUIRE(i1==fv1.begin());
    BOOST_REQUIRE(i2==fv2.begin());
}

BOOST_AUTO_TEST_CASE(test_async_wait_for_all1) {
    const int c=10;
    boost::chrono::system_clock::duration dur=boost::chrono::seconds(0);
    greenify_with_sched(scheduler(), [&](){
        auto start=boost::chrono::system_clock::now();
        std::vector<future<void>> fv1;
        for (int i=0; i<c; i++) {
            fv1.push_back(async([i](){
                this_thread::sleep_for(boost::chrono::milliseconds(100*(i+1)));
            }));
        }
        auto f1=async_wait_for_all(fv1.cbegin(), fv1.cend());

        std::vector<future<int>> fv2;
        for (int i=0; i<c; i++) {
            fv2.push_back(async([i](){
                this_thread::sleep_for(boost::chrono::milliseconds(90*(i+1)));
                return i;
            }));
        }
        auto f2=async_wait_for_all(fv2.cbegin(), fv2.cend());
        wait_for_all(f1, f2);
        auto stop=boost::chrono::system_clock::now();
        dur=stop-start;
    });
    BOOST_REQUIRE(dur>=boost::chrono::milliseconds(100*c));
}

BOOST_AUTO_TEST_CASE(test_then1) {
    std::string s;
    greenify_with_sched(scheduler(), [&](){
        promise<int> p;
        auto f0=p.get_future();
        auto f1=f0.then([](future<int> &f){ return boost::lexical_cast<std::string>(f.get()); });
        p.set_value(100);
        s=f1.get();
    });
    BOOST_REQUIRE(s==std::string("100"));
}

BOOST_AUTO_TEST_CASE(test_then2) {
    int n=0;
    greenify_with_sched(scheduler(), [&](){
        auto f=async([](){ return 100; })
            .then([](future<int> &f){ return boost::lexical_cast<std::string>(f.get()); })
            .then([](future<std::string> &f){ return boost::lexical_cast<int>(f.get()); })
        ;
        n=f.get();
    });
    BOOST_REQUIRE(n==100);
}

BOOST_AUTO_TEST_CASE(test_then3) {
    std::string s;
    greenify_with_sched(scheduler(), [&](){
        promise<int> p;
        auto f0=p.get_future().share();
        auto f1=f0.then([](shared_future<int> &f){ return boost::lexical_cast<std::string>(f.get()); });
        p.set_value(100);
        s=f1.get();
    });
    BOOST_REQUIRE(s==std::string("100"));
}

BOOST_AUTO_TEST_CASE(test_then4) {
    int n=0;
    greenify_with_sched(scheduler(), [&](){
        auto f=async([](){ return 100; }).share()
            .then([](shared_future<int> &f){ return boost::lexical_cast<std::string>(f.get()); }).share()
            .then([](shared_future<std::string> &f){ return boost::lexical_cast<int>(f.get()); }).share()
        ;
        n=f.get();
    });
    BOOST_REQUIRE(n==100);
}

BOOST_AUTO_TEST_CASE(test_packaged_task1) {
    int n=0;
    greenify_with_sched(scheduler(), [&](){
        packaged_task<int(int)> pt([](int x){ return x*10; });
        auto f=pt.get_future();
        pt(42);
        n=f.get();
    });
    BOOST_REQUIRE(n==420);
}

BOOST_AUTO_TEST_CASE(test_packaged_task2) {
    int x=0;
    greenify_with_sched(scheduler(), [&](){
        packaged_task<void()> pt([&x](){
            x++;
        });
        auto f=pt.get_future();
        pt();
        f.get();
    });
    BOOST_REQUIRE(x==1);
}

int thr_func(int x) {
    boost::this_thread::sleep_for(boost::chrono::seconds(1));
    return x*10;
}

void thr_func1() {
    boost::this_thread::sleep_for(boost::chrono::seconds(1));
}

void dot() {
    for (int i=0; i<30; i++) {
        this_thread::sleep_for(boost::chrono::milliseconds(100));
    }
}

BOOST_AUTO_TEST_CASE(test_foreign_thread_pool) {
    int n1=0;
    int n2=0;
    greenify_with_sched(scheduler(), [&](){
        thread f(dot);
        foreign_thread_pool pool;
        struct thr_op {
            int operator()(int x) {
                boost::this_thread::sleep_for(boost::chrono::seconds(1));
                return x*10;
            }
        };
        n1=pool(thr_func, 42);
        n2=pool(thr_op(), 42);
        pool(thr_func1);
        f.join();
    });
    BOOST_REQUIRE(n1==420);
    BOOST_REQUIRE(n2==420);
}
