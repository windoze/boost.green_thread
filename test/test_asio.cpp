#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>

#include <boost/asio/basic_waitable_timer.hpp>
#include <boost/chrono/system_clocks.hpp>
#include <boost/green_thread.hpp>
#define BOOST_DONT_GREENIFY_STD_STREAM
#define BOOST_DONT_GREENIFY_MAIN
#include <boost/green_thread/greenify.hpp>

using namespace boost::green_thread;

typedef boost::asio::basic_waitable_timer<boost::chrono::steady_clock> my_timer_t;

void canceler(my_timer_t &timer) {
    this_thread::sleep_for(boost::chrono::seconds(1));
    timer.cancel();
    this_thread::sleep_for(boost::chrono::seconds(1));
}

BOOST_AUTO_TEST_CASE(asio_with_yield) {
    boost::system::error_code ec;
    greenify_with_sched(scheduler(), [&ec](){
        my_timer_t timer(asio::get_io_service());
        // Async cancelation from another thread
        timer.expires_from_now(boost::chrono::seconds(3));
        thread f(thread::attributes(thread::attributes::stick_with_parent), canceler, std::ref(timer));
        timer.async_wait(asio::yield[ec]);
        f.join();
    });
    BOOST_REQUIRE(ec==boost::asio::error::make_error_code(boost::asio::error::operation_aborted));
}

BOOST_AUTO_TEST_CASE(asio_with_future) {
    future_status status=future_status::ready;
    greenify_with_sched(scheduler(), [&status](){
        my_timer_t timer(asio::get_io_service());
        // Use future
        timer.expires_from_now(boost::chrono::seconds(3));
        future<void> f=timer.async_wait(asio::use_future);
        status=f.wait_for(boost::chrono::seconds(1));
        assert(status==future_status::timeout);
    });
    BOOST_REQUIRE(status==future_status::timeout);
}
