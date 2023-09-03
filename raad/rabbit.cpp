////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2023 Dimitry Ishenko
// Contact: dimitry (dot) ishenko (at) (gee) mail (dot) com
//
// Distributed under the GNU GPL license. See the LICENSE.md file for details.

////////////////////////////////////////////////////////////////////////////////
#include "message.hpp"
#include "rabbit.hpp"
#include "serial.hpp"

#include <chrono>
#include <stdexcept>
#include <thread>

using namespace std::chrono_literals;
using namespace std::this_thread;

////////////////////////////////////////////////////////////////////////////////
asio::serial_port open_serial(asio::io_context& ctx, const std::string& name)
{
    asio::serial_port serial{ctx};
    do_("Opening serial port ", name, [&]{ serial = asio::serial_port{ctx, name}; });
    return serial;
}

////////////////////////////////////////////////////////////////////////////////
namespace
{

// ioi ld (WDTTR), 0x51
// ioi ld (WDTTR), 0x54
constexpr char disable_wd[] = "\x80\x09\x51\x80\x09\x54";

// ioi ld (GOCR), 0x30
constexpr char status_hi[] = "\x80\x0e\x30";

// ioi ld (GOCR), 0x20
constexpr char status_lo[] = "\x80\x0e\x20";

}

void reset_target(asio::serial_port& serial)
{
    do_("Resetting target", [&]{
        dtr(serial, hi);
        sleep_for(250ms);

        dtr(serial, lo);
        sleep_for(350ms);
    });
}

////////////////////////////////////////////////////////////////////////////////
void detect_target(asio::serial_port& serial)
{
    do_("Detecting presence", [&]{
        baud_rate(serial, 2400);

        // disable watchdog
        doing("W");
        asio::write(serial, asio::buffer(disable_wd, sizeof(disable_wd) - 1));

        // tell Rabbit to set the /STATUS pin high
        doing("H");
        asio::write(serial, asio::buffer(status_hi, sizeof(status_hi) - 1));
        sleep_for(100ms);
        // check the /STATUS pin
        if (dsr(serial)) throw std::runtime_error{"Target not responding"};

        // tell Rabbit to set the /STATUS pin low
        doing("L");
        asio::write(serial, asio::buffer(status_lo, sizeof(status_lo) - 1));
        sleep_for(100ms);
        // check the /STATUS pin
        if (!dsr(serial)) throw std::runtime_error{"Target not responding"};
    });
}
