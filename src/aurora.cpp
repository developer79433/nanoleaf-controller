#include <fstream>
#include <chrono>
#include <thread>

#include "aurora.h"

namespace mynanoleaf {

const char *Aurora::NANOLEAF_MDNS_SERVICE_TYPE = "_nanoleafapi._tcp";
const char *Aurora::API_PREFIX = "/api/v1/";

void Aurora::read_token_file() {
	std::ifstream fs;
	fs.open(TOKEN_FILENAME);
	fs >> token;
	fs.close();
}

void Aurora::write_token_file() {
	std::ofstream fs;
	fs.open(TOKEN_FILENAME);
	fs << token;
	fs.close();
}

size_t Aurora::accumulate_response(char *ptr, size_t size, size_t nmemb, void *userdata) {
	std::ostringstream *response_body = static_cast<std::ostringstream *>(userdata);
	response_body->write(ptr, size * nmemb);
	return size * nmemb;
}

void Aurora::generate_token() {
	std::ostringstream response_body;
	for (;;) {
		try {
			do_request("POST", "new", "/", NULL, response_body);
			std::cerr << "Authorisation successful" << std::endl;
			json j = json::parse(response_body.str());
			token = j["auth_token"];
			write_token_file();
		} catch (const std::string &errmsg) {
			std::cerr << errmsg << std::endl;
			std::cerr << "Authorisation failed; waiting before retry. Did you push and hold the controller button?" << std::endl;
			using namespace std::chrono_literals;
			std::this_thread::sleep_for(5s);
		}
	}
}

std::string Aurora::get_auth_token() {
	if (0 == token.length()) {
		read_token_file();
	}
	if (0 == token.length()) {
		generate_token();
	}
	if (0 != token.length()) {
		write_token_file();
	}
	return token;
}

void Aurora::do_request(
	const std::string &method,
	const std::string &token,
	const std::string &path,
	const std::string *request_body,
	std::ostringstream &response_body
) {
	std::ostringstream full_path;
	if (0 == token.length()) {
		throw std::string("No auth token");
	}
	full_path << API_PREFIX << token << path;
	curl.set_path(full_path.str());
	if (method == "GET") {
		curl.setopt(CURLOPT_HTTPGET, 1L);
		// Request body ignored
	} else if (method == "POST") {
		curl.setopt(CURLOPT_POSTFIELDSIZE, (NULL == request_body) ? 0L : request_body->size());
		curl.setopt(CURLOPT_POSTFIELDS, (NULL == request_body) ? "" : *request_body);
	}
	response_body.clear();
	curl.setopt(CURLOPT_WRITEDATA, &response_body);
	curl.setopt(CURLOPT_WRITEFUNCTION, accumulate_response);
	curl.setopt(CURLOPT_VERBOSE, 1L);
	curl.perform();
	if (200 == curl.get_status()) {
#ifndef NDEBUG
		std::cerr << "Request successful" << std::endl;
		std::cerr << "JSON response: " << response_body.str() << std::endl;
#endif /* ndef NDEBUG */
	} else {
#ifndef NDEBUG
		std::cerr << "Request failed: status " << curl.get_status() << std::endl;
#endif /* ndef NDEBUG */
		std::ostringstream msg;
		msg << "Unexpected HTTP status: " << curl.get_status() << std::endl;
		throw msg.str();
	}
}

void to_json(json &j, const ClampedValue &cv) {
	j = json{{"value", cv.value}, {"max", cv.max}, {"min", cv.min}};
}

void from_json(const json &j, ClampedValue &cv) {
	cv.value = j.at("value").get<int>();
	cv.max = j.at("max").get<int>();
	cv.min = j.at("min").get<int>();
}

void to_json(json& j, const Position& p) {
	j = json{{"x", p.x}, {"y", p.y}, {"o", p.o}};
}

void from_json(const json& j, Position& p) {
	p.x = j.at("x").get<int>();
	p.y = j.at("y").get<int>();
	p.o = j.at("o").get<int>();
}

void to_json(json& j, const PanelPosition& pp) {
	to_json(j, dynamic_cast<const Position &>(pp));
	j["panelId"] = pp.id;
}

void from_json(const json& j, PanelPosition& pp) {
	pp.id = j.at("panelId").get<int>();
	from_json(j, dynamic_cast<Position &>(pp));
}

void to_json(json &j, const Layout &l) {
	j = json{
		// The controller is counted in numPanels, but does not have an explicit panelPosition.
		{"numPanels", l.positions.size() + 1},
		{"sideLength", l.side_length},
		{"positionData", l.positions}
	};
}

void from_json(const json &j, Layout &l) {
	l.positions = j.at("positionData").get<std::vector<PanelPosition> >();
	l.side_length = j.at("sideLength");
	// The controller is counted in numPanels, but does not have an explicit panelPosition.
	if (j.at("numPanels") != l.positions.size() + 1) {
		std::ostringstream msg;
		msg << "Got numPanels==" << j.at("numPanels") << ", but " << l.positions.size() << " positionData elements";
		throw msg.str();
	}
}

void to_json(json &j, const PanelLayout &pl) {
	j = json{{"layout", pl.layout}, {"globalOrientation", pl.orientation}};
}

void from_json(const json &j, PanelLayout &pl) {
	pl.layout = j.at("layout").get<Layout>();
	pl.orientation = j.at("globalOrientation").get<GlobalOrientation>();
}

void to_json(json &j, const Effects &e) {
	j = json{{"select", e.current}, {"available", e.available}};
}

void from_json(const json &j, Effects &e) {
	e.current = j.at("select").get<std::string>();
	e.available = j.at("effectsList").get<std::vector<std::string> >();
}

void to_json(json &j, const State &s) {
	j = json{
		{"on", {
			"value", s.on
		}},
		{"brightness", s.brightness},
		{"sat", s.sat},
		{"ct", s.ct},
		{"colorMode", s.color_mode}
	};
}

void from_json(const json &j, State &s) {
	s.on = j.at("on").at("value").get<bool>();
	s.brightness = j.at("brightness").get<ClampedValue>();
	s.sat = j.at("sat").get<ClampedValue>();
	s.ct = j.at("ct").get<ClampedValue>();
	s.color_mode = j.at("colorMode").get<std::string>();
}

void to_json(json &j, const Rhythm &r) {
	j = json{
		{"rhythmConnected", r.connected},
		{"rhythmActive", r.active},
		{"rhythmId", r.id},
		{"hardwareVersion", r.hardware_version},
		{"firmwareVersion", r.firmware_version},
		{"auxAvailable", r.aux},
		{"rhythmMode", r.mode},
		{"rhythmPos", r.pos}
	};
}

void from_json(const json &j, Rhythm &r) {
	r.connected = j.at("rhythmConnected").get<bool>();
	r.active = j.at("rhythmActive").get<bool>();
	r.id = j.at("rhythmId").get<int>();
	r.hardware_version = j.at("hardwareVersion").get<std::string>();
	r.firmware_version = j.at("firmwareVersion").get<std::string>();
	r.aux = j.at("auxAvailable").get<bool>();
	r.mode = j.at("rhythmMode").get<int>();
	r.pos = j.at("rhythmPos").get<Position>();
}

void to_json(json &j, const AuroraJson &aj) {
	j = json{
		{"name", aj.name},
		{"serialNo", aj.serial_number},
		{"manufacturer", aj.manufacturer},
		{"firmwareVersion", aj.firmware_version},
		{"model", aj.model},
		{"state", aj.state},
		{"effects", aj.effects},
		{"panelLayout", aj.panel_layout},
		{"rhythm", aj.rhythm}
	};
}

void from_json(const json &j, AuroraJson &aj) {
	aj.name = j.at("name").get<std::string>();
	aj.serial_number = j.at("serialNo").get<std::string>();
	aj.manufacturer = j.at("manufacturer").get<std::string>();
	aj.firmware_version = j.at("firmwareVersion").get<std::string>();
	aj.model = j.at("model").get<std::string>();
	aj.state = j.at("state").get<State>();
	aj.effects = j.at("effects").get<Effects>();
	aj.panel_layout = j.at("panelLayout").get<PanelLayout>();
	aj.rhythm = j.at("rhythm").get<Rhythm>();
}

}
