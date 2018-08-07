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

#if 1
#define CATCH_EXCEPTIONS
#endif

void try_to_manipulate_aurora(const std::string &host, uint16_t port) {
#ifdef CATCH_EXCEPTIONS
	try {
#endif /* CATCH_EXCEPTIONS */
		mynanoleaf::Aurora aurora(host, port);
		aurora.manipulate();
#ifdef CATCH_EXCEPTIONS
	} catch (char const * const str) {
		std::cerr << "Aurora exception: " << str << std::endl;
	} catch (const std::string &sstr) {
		std::cerr << "Aurora exception: " << sstr << std::endl;
	}
#endif /* CATCH_EXCEPTIONS */
}
bool aurora_callback(const AvahiAddress &address, const std::string &host, uint16_t port, const std::string &id, void *userdata) {
	struct callback_args *args = static_cast<struct callback_args *>(userdata);
	bool ret;
	if (args->wanted_id == NULL || 0 == args->wanted_id->length()) {
		// Attempt to connect to all discovered devices
		std::cerr << "Discovered device ID '" << id << "'" << std::endl;
		try_to_manipulate_aurora(host, port);
		ret = true; // Continue enumeration
	} else if (id == *(args->wanted_id)) {
		// Found the sole device ID we're after
		std::cerr << "Discovered configured device ID '" << id << "'" << std::endl;
		try_to_manipulate_aurora(host, port);
		ret = false; // Stop enumeration
	} else {
		// Not the sole device ID we're after
		std::cerr << "Disregarding device ID '" << id << "'" << std::endl;
		ret = true; // Continue enumeration
	}
	return ret;
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
	mdns.discover(NANOLEAF_MDNS_SERVICE_TYPE, aurora_callback, &args);
	curl_global_cleanup();
	return EXIT_SUCCESS;
}
