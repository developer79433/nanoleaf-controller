#include <cstdlib>

#include "mycurlpp.h"
#include "aurora.h"
#include "discovery.h"
#include "streaming.h"

#if 1
#define AURORA_HOSTNAME "Nanoleaf-Light-Panels-53-3b-5d.local"
#endif

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

void try_to_manipulate_aurora(const std::string &host, int port = -1) {
#ifdef CATCH_EXCEPTIONS
	try {
#endif /* CATCH_EXCEPTIONS */
		mynanoleaf::Aurora *aurorap;
		if (-1 == port) {
			aurorap = new mynanoleaf::Aurora(host);
		} else {
			aurorap = new mynanoleaf::Aurora(host, port);
		}
		std::unique_ptr<mynanoleaf::Aurora> aurora(aurorap);
		aurora->get_info();
		std::string ipaddr, proto;
		uint16_t port;
		aurora->external_control(ipaddr, port, proto);
		do_external_control(*aurora, ipaddr, port, proto);
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
#if defined(AURORA_HOSTNAME)
	try_to_manipulate_aurora(AURORA_HOSTNAME);
#else /* ndef AURORA_HOSTNAME */
	struct callback_args args;
#ifdef AURORA_ID
	std::string wanted_id(AURORA_ID);
	args.wanted_id = &wanted_id;
#else /* ndef AURORA_ID */
	args.wanted_id = NULL;
#endif /* AURORA_ID */
	MDNSResponder mdns;
	mdns.discover(mynanoleaf::Aurora::NANOLEAF_MDNS_SERVICE_TYPE, aurora_callback, &args);
#endif /* ndef AURORA_HOSTNAME */

	curl_global_cleanup();

	return EXIT_SUCCESS;
}
