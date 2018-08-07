#include <iostream>
#include <sstream>
#include <fstream>
#include <chrono>
#include <thread>
#include <cstdlib>
#include <curl/curl.h>
#include <curl/easy.h>

#include "mycurlpp.h"
#include "aurora.h"
#include "discovery.h"

#define NANOLEAF_MDNS_SERVICE_TYPE "_nanoleafapi._tcp"

void aurora_callback(const AvahiAddress *address, const char *host, unsigned short port) {
	mynanoleaf::Aurora aurora(host, port);
	aurora.manipulate();
}

int
main(int argc, char *argv[])
{
	CURLcode res = curl_global_init(CURL_GLOBAL_ALL);
	if (res != CURLE_OK) {
		throw curl_easy_strerror(res);
	}
	std::cerr << __FILE__ << ":" << __LINE__ << " " << reinterpret_cast<void *>(aurora_callback) << std::endl;
	MDNSResponder mdns;
	try {
		mdns.discover(NANOLEAF_MDNS_SERVICE_TYPE, aurora_callback);
	} catch (char const * const str) {
		std::cerr << "Exception: " << str << std::endl;
	}
	curl_global_cleanup();
	return EXIT_SUCCESS;
}
