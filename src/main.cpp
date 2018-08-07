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
#if 0
/* Only use this Device ID */
#define AURORA_ID "74:A9:47:AD:62:C1"
#endif

struct callback_args {
	std::string *wanted_id;
};

void aurora_callback(const AvahiAddress &address, const std::string &host, uint16_t port, const std::string &id, void *userdata) {
	struct callback_args *args = static_cast<struct callback_args *>(userdata);
	mynanoleaf::Aurora aurora(host, port);
	if (args->wanted_id == NULL || 0 == args->wanted_id->length() || id == *(args->wanted_id)) {
		aurora.manipulate();
	}
}

int
main(int argc, char *argv[])
{
	CURLcode res = curl_global_init(CURL_GLOBAL_ALL);
	if (res != CURLE_OK) {
		throw curl_easy_strerror(res);
	}
	std::cerr << __FILE__ << ":" << __LINE__ << " " << reinterpret_cast<void *>(aurora_callback) << std::endl;
	struct callback_args args;
#ifdef AURORA_ID
	std::string wanted_id(AURORA_ID);
	args.wanted_id = &wanted_id;
#else /* ndef AURORA_ID */
	args.wanted_id = NULL;
#endif /* AURORA_ID */
	MDNSResponder mdns;
	try {
		mdns.discover(NANOLEAF_MDNS_SERVICE_TYPE, aurora_callback, &args);
	} catch (char const * const str) {
		std::cerr << "Exception: " << str << std::endl;
	}
	curl_global_cleanup();
	return EXIT_SUCCESS;
}
