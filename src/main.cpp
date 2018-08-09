#include <cstdlib>

#include "aurora.h"

#if 1
#define AURORA_HOSTNAME "Nanoleaf-Light-Panels-53-3b-5d.local"
#endif

#if 0
/* Only use this Device ID */
#define AURORA_ID "74:A9:47:AD:62:C1"
#endif

void do_external_control(mynanoleaf::Aurora &aurora, mynanoleaf::IPStream &stream) {
	std::vector<mynanoleaf::PanelCommand> commands;
	for (auto &p: aurora.get_panel_positions()) {
		std::vector<mynanoleaf::Frame> frames;
		mynanoleaf::Frame *f = new mynanoleaf::Frame(0xa0, 0x52, 0x2d, 1);
		frames.push_back(*f);
		auto *pc = new mynanoleaf::PanelCommand(p.id, frames);
		commands.push_back(*pc);
	}
	write_panel_commands(stream, commands);
}

#if 1
#define CATCH_EXCEPTIONS
#endif

void try_to_manipulate_aurora(mynanoleaf::Aurora &aurora) {
#ifdef CATCH_EXCEPTIONS
	try {
#endif /* CATCH_EXCEPTIONS */
		aurora.get_info();
		mynanoleaf::IPStream &sock = aurora.external_control();
		do_external_control(aurora, sock);
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
