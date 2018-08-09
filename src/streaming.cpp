#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <sstream>
#include <cstdlib>
#include <cerrno>
#include <cstring>

#include "streaming.h"
#include "util.h"

namespace mynanoleaf {

IPStream::IPStream(
	const std::string &ipaddr,
	uint16_t port,
	int sock_type,
	int sock_proto
) {
	fd = socket(AF_INET, sock_type, sock_proto);
	if (fd < 0) {
		throw std::string(std::strerror(errno));
	}
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	int ret = inet_aton(ipaddr.c_str(), &addr.sin_addr);
	if (ret < 0) {
		throw std::string(std::strerror(errno));
	}
	addr.sin_port = htons(port);
	ret = connect(fd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr));
	if (ret < 0) {
		throw std::string(strerror(errno));
	}
}

IPStream::~IPStream() {
	if (fd >= 0) {
		int ret = shutdown(fd, SHUT_RDWR);
		if (ret < 0) {
			std::cerr << "shutdown: " << strerror(errno) << std::endl;
		}
		ret = close(fd);
		if (ret < 0) {
			std::cerr << "close: " << strerror(errno) << std::endl;
		}
		fd = -1;
	}
}

static std::string hexdump(const void *p, size_t n) {
	static const char *hexdigits = "0123456789abcdef";
	const unsigned char *pc = static_cast<const unsigned char *>(p);
	std::ostringstream ret;
	for (; n; --n, ++pc) {
		if (
			(*pc >= 'a' && *pc <= 'z') ||
			(*pc >= 'A' && *pc <= 'Z') ||
			(*pc >= '0' && *pc <= '9')
		) {
			ret << *pc;
		} else {
			ret << "\\x" << hexdigits[*pc >> 4] << hexdigits[*pc & 0x0F];
		}
	}
	return ret.str();
}

void IPStream::write(const void *p, size_t n) {
	if (n) {
		std::cerr << "Writing: " << hexdump(p, n) << std::endl;
	}
	while (n > 0) {
		int ret = ::write(fd, p, n);
		if (ret < 0) {
			throw std::string(strerror(errno));
		}
		p = static_cast<const unsigned char *>(p) + ret;
		n -= ret;
	}
}

IPStream *IPStream::create(const std::string &proto, const std::string &ipaddr, uint16_t port) {
	if (proto == "udp") {
		return new BufferedUDPStream(ipaddr, port);
	} else if (proto == "tcp") {
		return new TCPStream(ipaddr, port);
	} else {
		std::ostringstream msg;
		msg << "Unrecognised protocol '" << proto << "'";
		throw std::string(msg.str());
	}
}

UDPStream::UDPStream(
	const std::string &ipaddr,
	uint16_t port
) : IPStream(ipaddr, port, SOCK_DGRAM, IPPROTO_UDP) {
}

TCPStream::TCPStream(
	const std::string &ipaddr,
	uint16_t port
) : IPStream(ipaddr, port, SOCK_STREAM, IPPROTO_TCP) {
}

void write_panel_commands(IPStream &stream, const std::vector<PanelCommand> &commands) {
	assert(commands.size() < 256);
	stream.write(static_cast<uint8_t>(commands.size()));
	for (auto &c: commands) {
		c.write(stream);
	}
	stream.flush();
}

}
