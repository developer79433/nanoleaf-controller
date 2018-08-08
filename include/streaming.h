#ifndef STREAMING_H
#define STREAMING_H

#include <string>

#include "aurora.h"

namespace mynanoleaf {

class IPStream {
private:
	int fd;
public:
	IPStream(
		const std::string &ipaddr,
		uint16_t port,
		int sock_type,
		int sock_proto
	);
	virtual ~IPStream();
	virtual void write(uint8_t b) {
		write(&b, sizeof(b));
	}
	virtual void write(const void *p, size_t n);
	virtual void flush() {}
	static IPStream *create(const std::string &proto, const std::string &ipaddr, uint16_t port);
};

class UDPStream : public IPStream {
public:
	UDPStream(
		const std::string &ipaddr,
		uint16_t port
	);
	virtual ~UDPStream() {}
};

class BufferedUDPStream : public UDPStream {
private:
	std::ostringstream buf;
public:
	BufferedUDPStream(
		const std::string &ipaddr,
		uint16_t port
	) : UDPStream(ipaddr, port) {
	}
	virtual ~BufferedUDPStream() {
		flush();
	}
	virtual void write(const void *p, size_t n) {
		buf.write(static_cast<const char *>(p), n);
	}
	virtual void flush() {
		UDPStream::write(buf.str().c_str(), buf.str().size());
		buf.str("");
	}
};

class TCPStream : public IPStream {
public:
	TCPStream(
		const std::string &ipaddr,
		uint16_t port
	);
	virtual ~TCPStream() {}
};

class Frame {
private:
	uint8_t r, g, b, t;
public:
	Frame(uint8_t pr, uint8_t pg, uint8_t pb, uint8_t pt)
	:
		r(pr), g(pg), b(pb), t(pt)
	{}
	virtual ~Frame() {}
	void write(IPStream &stream) const {
		stream.write(r);
		stream.write(g);
		stream.write(b);
		stream.write(0); // White; ignored
		stream.write(t);
	}
};

class PanelCommand {
private:
	uint8_t panel_id;
	std::vector<Frame> frames;
public:
	PanelCommand(uint8_t ppanel_id) : panel_id(ppanel_id) {}
	PanelCommand(uint8_t ppanel_id, const std::vector<Frame> &pframes) : panel_id(ppanel_id), frames(pframes) {}
	virtual ~PanelCommand() {}
	void write(IPStream &stream) const {
		stream.write(panel_id);
		assert(frames.size() < 256);
		stream.write(static_cast<uint8_t>(frames.size()));
		for (auto &f: frames) {
			f.write(stream);
		}
	}
};

void do_external_control(Aurora &aurora, const std::string &ipaddr, uint16_t port, const std::string &proto);

}

#endif /* STREAMING_H */
