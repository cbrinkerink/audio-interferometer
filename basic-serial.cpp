#include <iostream>
#include <sstream>
#include <iomanip>
#include <boost/asio.hpp> 
#include <termios.h>
using namespace std;
using namespace boost;

// serial over USB port for ECP5 Versa: /dev/tty.usbserial-FT1V75R11

// TODO:
// - Figure out how to sync. We mostly get packets that line up with the baselines,
//   but not always - this is to be expected because of the relatively low duty
//   cycle we currently use. Duty cycle will be higher for more baselines and lags,
//   so we need to address this now.
//   Idea: start by simply reading byte-by-byte until we find the 'DCBA' sequence,
//   then adjust our reading cadence so that we get neat packets (first read 252
//   bytes, then read in packets of 256 bytes).
// - Package the received data into a structure which we can feed into the texture.

std::string uchar2hex(unsigned char inchar)
{
  std::ostringstream oss (std::ostringstream::out);
  oss << std::setw(2) << std::setfill('0') << std::hex << (int)(inchar);
  return oss.str();
}

int main(int argc, char* argv[])
{
    asio::io_service io;
    asio::serial_port port(io);

    port.open("/dev/tty.usbserial-FT1V75R11");
    termios t;
    int fd = port.native_handle();

    // Set the baud rate manually here
    if (tcgetattr(fd, &t) < 0) { /* handle error */ }
    if (cfsetspeed(&t, 921600) < 0) { /* handle error */ }
    if (tcsetattr(fd, TCSANOW, &t) < 0) { /* handle error */ }

    // First, let's sync with the packet train we receive
    bool synced = false;
    char tmp;
    int state = 0;
    char lookingfor[] = {68, 67, 66, 65};
    while (!synced) {
        asio::read(port, asio::buffer(&tmp, 1));
        if (tmp == lookingfor[state]) {
	    state++;
	} else {
	    state = 0;
	}
	if (state == 4) synced = true;
    }
    char tmp2[252 + 5 * 256];
    asio::read(port, asio::buffer(&tmp2, 252 + 5 * 256));

    // Now, we are synced! We can receive packets normally.
    char buf[256];
    int packetnum = 0;

    while (true)
    {
        asio::read(port, asio::buffer(&buf, 256));
	ostringstream oss (ostringstream::out);
	for (int i = 0; i < 256; i++) {
          oss << uchar2hex(buf[i]) << " ";
	}
	std::cout << packetnum << std::endl;
	std::cout << oss.str() << std::endl;
	packetnum++;
	packetnum = packetnum % 6;
    }

    port.close();

    std::cin.get();
}
