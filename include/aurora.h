#ifndef AURORA_H
#define AURORA_H 1

#include <nlohmann/json.hpp>

#include "mycurlpp.h"

#define TOKEN_FILENAME "auth_token.dat"

namespace mynanoleaf {

using json = nlohmann::json;

class ClampedValue {
public:
	int value, max, min;
};

class Position {
public:
	int x, y, o;
};

class PanelPosition : public Position {
public:
	int id;
};

class Layout {
public:
	int side_length;
	std::vector<PanelPosition> positions;
};

class GlobalOrientation : public ClampedValue {
};

class PanelLayout {
public:
	Layout layout;
	GlobalOrientation orientation;
};

class Effects {
public:
	std::string current;
	std::vector<std::string> available;
};

class State {
public:
	bool on;
	ClampedValue brightness, hue, sat, ct;
	std::string color_mode;
};

class Rhythm {
public:
	bool connected, active;
	int id;
	std::string hardware_version, firmware_version;
	bool aux;
	int mode;
	Position pos;
};

class AuroraJson {
public:
	std::string name;
	std::string serial_number;
	std::string manufacturer;
	std::string firmware_version;
	std::string model;
	State state;
	Effects effects;
	PanelLayout panel_layout;
	Rhythm rhythm;
};

class Aurora {
public:
	static const char *NANOLEAF_MDNS_SERVICE_TYPE;
	static const char *API_PREFIX;
private:
	mycurlpp::Curl curl;
	std::string token;
	AuroraJson all_info;
	void read_token_file();
	void write_token_file();
	void do_request(
		const std::string &method,
		const std::string &token,
		const std::string &path,
		const std::string *request_body,
		std::ostringstream &response_body
	);
public:
	Aurora(const std::string &hostname, unsigned short port = 16021) : curl(mycurlpp::Curl(hostname, port)) {}
	virtual ~Aurora() {}
	static size_t accumulate_response(const char *ptr, size_t size, size_t nmemb, void *userdata);
	static size_t stream_request(char *ptr, size_t size, size_t nmemb, void *userdata);
	void generate_token();
	std::string get_auth_token();
	unsigned int get_panel_count() const {
		return all_info.panel_layout.layout.positions.size();
	}
	const std::vector<PanelPosition> &get_panel_positions() const {
		return all_info.panel_layout.layout.positions;
	}
	void get_info() {
		std::ostringstream response_body;
		do_request("GET", get_auth_token(), "/", NULL, response_body);
		all_info = json::parse(response_body.str());
		std::cerr << "Panel count: " << get_panel_count() << std::endl;
	}
	void external_control(
		std::string &ipaddr,
		uint16_t &port,
		std::string &proto
	);
};

void to_json(json &j, const ClampedValue &cv);
void from_json(const json &j, ClampedValue &cv);

void to_json(json &j, const Position &p);
void from_json(const json &j, Position &p);

void to_json(json &j, const Layout &p);
void from_json(const json &j, Layout &p);

void to_json(json &j, const PanelLayout &p);
void from_json(const json &j, PanelLayout &p);

void to_json(json &j, const Effects &e);
void from_json(const json &j, Effects &e);

void to_json(json &j, const State &s);
void from_json(const json &j, State &s);

void to_json(json &j, const Rhythm &r);
void from_json(const json &j, Rhythm &r);

void to_json(json &j, const AuroraJson &aj);
void from_json(const json &j, AuroraJson &aj);

}

#endif /* AURORA_H */
