#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>

#include <boost/random.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/chrono/system_clocks.hpp>
#include <boost/green_thread.hpp>
#define BOOST_DONT_GREENIFY_STD_STREAM
#define BOOST_DONT_GREENIFY_MAIN
#include <boost/green_thread/greenify.hpp>

using namespace boost::green_thread;

void child() {
    this_thread::sleep_for(boost::chrono::seconds(1));
    tcp_stream str;
    boost::system::error_code ec=str.connect("127.0.0.1:12345");
    assert(!ec);
    str << "hello" << std::endl;
    for(int i=0; i<100; i++) {
        // Receive a random number from server and send it back
        std::string line;
        std::getline(str, line);
        int n=boost::lexical_cast<int>(line);
        str << n << std::endl;
    }
    str.close();
}

void parent() {
    thread f(child);
    boost::random::mt19937 rng;
    boost::random::uniform_int_distribution<> rand(1,1000);

    tcp_stream_acceptor acc("127.0.0.1:12345");
    boost::system::error_code ec;
    tcp_stream str;
    acc(str, ec);
    assert(!ec);
    std::string line;
    std::getline(str, line);
    BOOST_REQUIRE(line=="hello");
    for (int i=0; i<100; i++) {
        // Ping client with random number
        int n=rand(rng);
        str << n << std::endl;
        std::getline(str, line);
        int r=boost::lexical_cast<int>(line);
        BOOST_REQUIRE(n==r);
    }
    str.close();
    acc.close();
    
    f.join();
}

BOOST_AUTO_TEST_CASE(test_tcp_stream) {
    greenify_with_sched(scheduler(), [](){
        thread_group threads;
        threads.create_thread(parent);
        threads.join_all();
    });
}
