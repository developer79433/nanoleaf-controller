#ifndef MYCURLPP_H
#define MYCURLPP_H 1

#include <iostream>
#include <sstream>
#include <curl/curl.h>
#include <curl/easy.h>

namespace mycurlpp {

class Curl {
private:
	CURL *curl;
	std::string hostname;
	unsigned int port;
	bool use_ssl;
	std::string path;
	long last_response_code;
public:
	Curl(
		const char *phostname = NULL,
		unsigned int pport = 80,
		bool puse_ssl = false,
		const char *ppath = "/",
		long plast_response_code = 0
	) :
		curl(curl_easy_init()),
		hostname(phostname),
		port(pport),
		use_ssl(puse_ssl),
		path(ppath),
		last_response_code(plast_response_code)
	{
	}
	Curl(
		const std::string &phostname,
		unsigned int pport = 80,
		bool puse_ssl = false,
		const char *ppath = "/",
		long plast_response_code = 0
	) :
		curl(curl_easy_init()),
		hostname(phostname),
		port(pport),
		use_ssl(puse_ssl),
		path(ppath),
		last_response_code(plast_response_code)
	{
	}
	virtual ~Curl() {
		if (curl) {
			curl_easy_cleanup(curl);
		}
	}
	void set_hostname(std::string &new_hostname) {
		hostname = new_hostname;
	}
	void set_port(unsigned short new_port) {
		port = new_port;
	}
	void set_ssl(bool new_ssl) {
		use_ssl = new_ssl;
	}
	void set_path(const std::string &new_path) {
		path = new_path;
	}
	void make_url(
		std::ostringstream &url
	) {
		url.clear();
		url << "http" << (use_ssl ? "s" : "") << "://" << hostname <<
				":" << port << path;
	}
	std::string get_url(void) {
		std::ostringstream url;
		make_url(url);
		return url.str();
	}
	operator CURL*(void) { return curl; }
	operator const CURL*(void) { return curl; }
	template<typename T> void setopt(CURLoption opt, const T& val) {
		CURLcode res;
		res = curl_easy_setopt(curl, opt, val);
		if (res != CURLE_OK) {
			throw curl_easy_strerror(res);
		}
	}
	unsigned int get_status(void) const { return static_cast<unsigned int>(last_response_code); }
	unsigned int perform(void) {
		std::ostringstream surl;
		make_url(surl);
		const std::string &url = surl.str();
		std::cerr << "Connecting to '" << url << "'" << std::endl;
		setopt(CURLOPT_URL, url.c_str());
		CURLcode res;
		res = curl_easy_perform(curl);
		if (res == CURLE_OK) {
			curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &last_response_code);
		} else {
			throw curl_easy_strerror(res);
		}
		return get_status();
	}
};

}

#endif /* MYCURLPP_H */
