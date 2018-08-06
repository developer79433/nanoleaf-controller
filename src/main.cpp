#include <iostream>
#include <sstream>
#include <fstream>
#include <chrono>
#include <thread>
#include <cstdlib>
#include <curl/curl.h>
#include <curl/easy.h>
#include <avahi-client/client.h>
#include <avahi-client/lookup.h>
#include <avahi-common/simple-watch.h>
#include <avahi-common/malloc.h>
#include <avahi-common/error.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

#define NANOLEAF_MDNS_SERVICE_TYPE "_nanoleafapi._tcp"

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
		const char *ppath = "/"
	) :
		curl(curl_easy_init()),
		hostname(phostname),
		port(pport),
		use_ssl(puse_ssl),
		path(ppath)
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

#define TOKEN_FILENAME "auth_token.dat"

void
read_token_file(std::string &tok)
{
	std::ifstream fs;
	fs.open(TOKEN_FILENAME);
	fs >> tok;
	fs.close();
}

void
write_token_file(const std::string &tok) {
	std::ofstream fs;
	fs.open(TOKEN_FILENAME);
	fs << tok;
	fs.close();
}

size_t accumulate_response(char *ptr, size_t size, size_t nmemb, void *userdata)
{
	std::ostringstream *response_body = static_cast<std::ostringstream *>(userdata);
	response_body->write(ptr, size * nmemb);
	return size * nmemb;
}

void
generate_token(Curl &curl, std::string &token)
{
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
			// TODO: Parse JSON response into token
			json j = json::parse(response_body.str());
			token = j["auth_token"];
			break;
		} else if (403 == curl.get_status()) {
			std::cerr << "Authorisation failed; waiting before retry. Did you push and hold the controller button?" << std::endl;
			using namespace std::chrono_literals;
			std::this_thread::sleep_for(5s);
		} else {
			std::ostringstream msg;
			msg << "Unrecognised HTTP status; " << curl.get_status() << std::endl;
			throw msg.str().c_str();
		}
	}
}

std::string get_auth_token(Curl &curl)
{
	static std::string token;
	if (0 == token.length()) {
		read_token_file(token);
	}
	if (0 == token.length()) {
		generate_token(curl, token);
	}
	if (0 != token.length()) {
		write_token_file(token);
	}
	return token;
}

void
manipulate_panels(Curl &curl)
{
	std::ostringstream path;
	std::string token = get_auth_token(curl);
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
		msg << "Unrecognised HTTP status; " << curl.get_status() << std::endl;
		throw msg.str().c_str();
	}
}

struct resolve_callback_args {
	AvahiClient *client;
};

static AvahiSimplePoll *simple_poll = NULL;
static void resolve_callback(
    AvahiServiceResolver *r,
    AVAHI_GCC_UNUSED AvahiIfIndex interface,
    AVAHI_GCC_UNUSED AvahiProtocol protocol,
    AvahiResolverEvent event,
    const char *name,
    const char *type,
    const char *domain,
    const char *host_name,
    const AvahiAddress *address,
    uint16_t port,
    AvahiStringList *txt,
    AvahiLookupResultFlags flags,
    void* userdata) {
    assert(r);
    resolve_callback_args *args = static_cast<resolve_callback_args *>(userdata);
    (void) args;
    /* Called whenever a service has been resolved successfully or timed out */
    switch (event) {
        case AVAHI_RESOLVER_FAILURE:
            throw avahi_strerror(avahi_client_errno(avahi_service_resolver_get_client(r)));
        case AVAHI_RESOLVER_FOUND: {
            char a[AVAHI_ADDRESS_STR_MAX], *t;
            std::cerr << "Service '" << name << "' of type '" << type << "' in domain '" << domain << "':" << std::endl;
            avahi_address_snprint(a, sizeof(a), address);
            t = avahi_string_list_to_string(txt);
            std::cerr <<
                    "\t" << host_name << ":" << port << " (" << a << ")" << std::endl <<
                    "\tTXT=" << t << std::endl <<
                    "\tcookie is " << avahi_string_list_get_service_cookie(txt) << std::endl <<
                    "\tis_local: " << !!(flags & AVAHI_LOOKUP_RESULT_LOCAL) << std::endl <<
                    "\tour_own: " << !!(flags & AVAHI_LOOKUP_RESULT_OUR_OWN) << std::endl <<
                    "\twide_area: " << !!(flags & AVAHI_LOOKUP_RESULT_WIDE_AREA) << std::endl <<
                    "\tmulticast: " << !!(flags & AVAHI_LOOKUP_RESULT_MULTICAST) << std::endl <<
                    "\tcached: " << !!(flags & AVAHI_LOOKUP_RESULT_CACHED) << std::endl
			;
            Curl curl(host_name, port);
            manipulate_panels(curl);
            avahi_free(t);
        }
    }
    avahi_service_resolver_free(r);
}

struct browse_callback_args {
	AvahiClient *client;
};

static void browse_callback(
    AvahiServiceBrowser *b,
    AvahiIfIndex interface,
    AvahiProtocol protocol,
    AvahiBrowserEvent event,
    const char *name,
    const char *type,
    const char *domain,
    AVAHI_GCC_UNUSED AvahiLookupResultFlags flags,
    void* userdata) {
	browse_callback_args *args = static_cast<browse_callback_args *>(userdata);
    assert(b);
    /* Called whenever a new services becomes available on the LAN or is removed from the LAN */
    const char *errmsg;
    switch (event) {
        case AVAHI_BROWSER_FAILURE:
        	errmsg = avahi_strerror(avahi_client_errno(avahi_service_browser_get_client(b)));
            avahi_simple_poll_quit(simple_poll);
            throw errmsg;
        case AVAHI_BROWSER_NEW:
            std::cerr << "(Browser) NEW: service '" << name << "' of type '" << type << "' in domain '" << domain << "'" << std::endl;
            /* We ignore the returned resolver object. In the callback
               function we free it. If the server is terminated before
               the callback function is called the server will free
               the resolver for us. */
            struct resolve_callback_args resolve_args;
            resolve_args.client = args->client;
            if (!(avahi_service_resolver_new(args->client, interface, protocol, name, type, domain, AVAHI_PROTO_UNSPEC, AVAHI_LOOKUP_USE_MULTICAST, resolve_callback, &resolve_args)))
                throw avahi_strerror(avahi_client_errno(args->client));
            break;
        case AVAHI_BROWSER_REMOVE:
#ifndef NDEBUG
            std::cerr << "(Browser) REMOVE: service '" << name << "' of type '" << type << "' in domain '" << domain << "'" << std::endl;
#endif /* ndef NDEBUG */
            break;
        case AVAHI_BROWSER_ALL_FOR_NOW:
        case AVAHI_BROWSER_CACHE_EXHAUSTED:
#ifndef NDEBUG
            std::cerr << "(Browser) " << (event == AVAHI_BROWSER_CACHE_EXHAUSTED ? "CACHE_EXHAUSTED" : "ALL_FOR_NOW") << std::endl;
#endif /* ndef NDEBUG */
            break;
    }
}

struct client_callback_args {
	AvahiClient *client;
};

static void client_callback(AvahiClient *c, AvahiClientState state, AVAHI_GCC_UNUSED void *userdata) {
	client_callback_args *args = static_cast<client_callback_args *>(userdata);
	(void) args;
    assert(c);
    /* Called whenever the client or server state changes */
    if (state == AVAHI_CLIENT_FAILURE) {
        const char *errmsg = avahi_strerror(avahi_client_errno(c));
        avahi_simple_poll_quit(simple_poll);
        throw errmsg;
    }
}

void
discover(void)
{
	int error;
    /* Allocate main loop object */
    if (!(simple_poll = avahi_simple_poll_new())) {
        throw avahi_strerror(error);
    }
    const AvahiPoll *poll = avahi_simple_poll_get(simple_poll);
    struct client_callback_args client_args;
    client_args.client = avahi_client_new(poll, AVAHI_CLIENT_NO_FAIL, client_callback, &client_args, &error);
    if (!client_args.client) {
        throw avahi_strerror(error);
    }
    /* Create the service browser */
    browse_callback_args browse_args;
    browse_args.client = client_args.client;
    AvahiServiceBrowser *sb = avahi_service_browser_new(
    	client_args.client,
		AVAHI_IF_UNSPEC,
		AVAHI_PROTO_UNSPEC,
		NANOLEAF_MDNS_SERVICE_TYPE,
		NULL,
		AVAHI_LOOKUP_USE_MULTICAST,
		browse_callback,
		&browse_args
	);
    if (!sb) {
        throw avahi_strerror(avahi_client_errno(client_args.client));
    }
    /* Run the main loop */
    avahi_simple_poll_loop(simple_poll);
}

int
main(int argc, char *argv[])
{
	CURLcode res = curl_global_init(CURL_GLOBAL_ALL);
	if (res != CURLE_OK) {
		throw curl_easy_strerror(res);
	}
	try {
		discover();
	} catch (char const * const str) {
		std::cerr << "Exception: " << str << std::endl;
	}
	curl_global_cleanup();
	return EXIT_SUCCESS;
}
