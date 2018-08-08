#include <cstdlib>

#include "mycurlpp.h"
#include "aurora.h"
#include "streaming.h"

#if 1
#define AURORA_HOSTNAME "Nanoleaf-Light-Panels-53-3b-5d.local"
#endif

#if 0
/* Only use this Device ID */
#define AURORA_ID "74:A9:47:AD:62:C1"
#endif

#if 1
#define CATCH_EXCEPTIONS
#endif

void try_to_manipulate_aurora(mynanoleaf::Aurora &aurora) {
#ifdef CATCH_EXCEPTIONS
	try {
#endif /* CATCH_EXCEPTIONS */
		aurora.get_info();
		std::string ipaddr, proto;
		uint16_t port;
		aurora.external_control(ipaddr, port, proto);
		do_external_control(aurora, ipaddr, port, proto);
#ifdef CATCH_EXCEPTIONS
	} catch (char const * const str) {
		std::cerr << "Aurora exception: " << str << std::endl;
	} catch (const std::string &sstr) {
		std::cerr << "Aurora exception: " << sstr << std::endl;
	}
#endif /* CATCH_EXCEPTIONS */
}

int
main(int argc, char *argv[])
{
	CURLcode res = curl_global_init(CURL_GLOBAL_ALL);
	if (res != CURLE_OK) {
		throw curl_easy_strerror(res);
	}

#if defined(AURORA_HOSTNAME)
	new mynanoleaf::Aurora(AURORA_HOSTNAME);
#else /* ndef AURORA_HOSTNAME */
	const std::string *wanted_id_p;
#ifdef AURORA_ID
	std::string wanted_id(AURORA_ID);
	wanted_id_p = &wanted_id;
#else /* ndef AURORA_ID */
	wanted_id_p = NULL;
#endif /* AURORA_ID */
	mynanoleaf::Aurora::discover(wanted_id_p);
#endif /* ndef AURORA_HOSTNAME */
	for (mynanoleaf::Aurora *aurora: mynanoleaf::Aurora::get_instances()) {
		try_to_manipulate_aurora(*aurora);
	}
	for (mynanoleaf::Aurora *aurora: mynanoleaf::Aurora::get_instances()) {
		delete aurora;
	}

	curl_global_cleanup();

	return EXIT_SUCCESS;
}
