#ifndef AURORA_H
#define AURORA_H 1

#include <nlohmann/json.hpp>

#include "mycurlpp.h"

using json = nlohmann::json;

#define TOKEN_FILENAME "auth_token.dat"

namespace mynanoleaf {

class Aurora {
public:
	static const char *NANOLEAF_MDNS_SERVICE_TYPE;
private:
	mycurlpp::Curl curl;
	std::string token;
	void read_token_file();
	void write_token_file();
public:
	Aurora(const std::string &hostname, unsigned short port) : curl(mycurlpp::Curl(hostname, port)) {}
	virtual ~Aurora() {}
	static size_t accumulate_response(char *ptr, size_t size, size_t nmemb, void *userdata);
	void generate_token();
	std::string get_auth_token();
	void get_info();
};

}

#endif /* AURORA_H */
