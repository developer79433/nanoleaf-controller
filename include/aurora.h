#ifndef AURORA_H
#define AURORA_H 1

#include <nlohmann/json.hpp>

using json = nlohmann::json;

#define TOKEN_FILENAME "auth_token.dat"

namespace mynanoleaf {

class Aurora {
private:
	mycurlpp::Curl curl;
	std::string token;
	void read_token_file() {
		std::ifstream fs;
		fs.open(TOKEN_FILENAME);
		fs >> token;
		fs.close();
	}
	void write_token_file() {
		std::ofstream fs;
		fs.open(TOKEN_FILENAME);
		fs << token;
		fs.close();
	}
public:
	Aurora(const std::string &hostname, unsigned short port) : curl(mycurlpp::Curl(hostname, port)) {}
	virtual ~Aurora() {}
	static size_t accumulate_response(char *ptr, size_t size, size_t nmemb, void *userdata) {
		std::ostringstream *response_body = static_cast<std::ostringstream *>(userdata);
		response_body->write(ptr, size * nmemb);
		return size * nmemb;
	}
	void generate_token() {
		curl.set_path("/api/v1/new");
		curl.setopt(CURLOPT_POSTFIELDSIZE, 0L);
		curl.setopt(CURLOPT_POSTFIELDS, "");
		std::ostringstream response_body;
		curl.setopt(CURLOPT_WRITEDATA, &response_body);
		curl.setopt(CURLOPT_WRITEFUNCTION, accumulate_response);
		curl.setopt(CURLOPT_VERBOSE, 1L);
		for (;;) {
			curl.perform();
			if (200 == curl.get_status()) {
				std::cerr << "Authorisation successful" << std::endl;
				json j = json::parse(response_body.str());
				token = j["auth_token"];
				write_token_file();
				break;
			} else if (403 == curl.get_status()) {
				std::cerr << "Authorisation failed; waiting before retry. Did you push and hold the controller button?" << std::endl;
				using namespace std::chrono_literals;
				std::this_thread::sleep_for(5s);
			} else {
				std::ostringstream msg;
				msg << "Unrecognised HTTP status: " << curl.get_status() << std::endl;
				throw msg.str();
			}
		}
	}
	std::string get_auth_token() {
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
	void manipulate() {
		std::ostringstream path;
		std::string token = get_auth_token();
		if (0 == token.length()) {
			throw "No auth token";
		}
		path << "/api/v1/" << token << "/";
		curl.set_path(path.str());
		curl.setopt(CURLOPT_HTTPGET, 1L);
		std::ostringstream response_body;
		curl.setopt(CURLOPT_WRITEDATA, &response_body);
		curl.setopt(CURLOPT_WRITEFUNCTION, accumulate_response);
		curl.setopt(CURLOPT_VERBOSE, 1L);
		curl.perform();
		if (200 == curl.get_status()) {
			std::cerr << "Get All Light Panel Controller Info successful" << std::endl;
			// TODO: Parse JSON response into token
			json j = json::parse(response_body.str());
			std::cerr << " Panel count: " << j["panelLayout"]["layout"]["numPanels"] << std::endl;
		} else {
			std::ostringstream msg;
			msg << "Unrecognised HTTP status: " << curl.get_status() << std::endl;
			throw msg.str();
		}
	}
};

}

#endif /* AURORA_H */
