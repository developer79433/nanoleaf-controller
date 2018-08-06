#include <iostream>
#include <sstream>
#include <cstdlib>
#include <curl/curl.h>
#include <curl/easy.h>
#include <avahi-client/client.h>
#include <avahi-client/lookup.h>
#include <avahi-common/simple-watch.h>
#include <avahi-common/malloc.h>
#include <avahi-common/error.h>

#define NANOLEAF_MDNS_SERVICE_TYPE "_nanoleafapi._tcp"
#define URL "http://192.168.1.17/"

void
make_url(std::ostringstream &url, const char *host_name, unsigned short port, bool use_ssl = false)
{
	url << "http" << (use_ssl ? "s" : "") << "://" << host_name <<
			":" << port << "/";
}

size_t write_callback(char *ptr, size_t size, size_t nmemb, void *userdata)
{
	std::cerr.write(ptr, size * nmemb);
	return size * nmemb;
}

void
do_connect_to_panels(CURL *curl, const char *host_name, unsigned short port)
{
	std::ostringstream url;
	make_url(url, host_name, port);
	std::cerr << "Connecting to '" << url.str().c_str() << "'" << std::endl;
	CURLcode res;
	res = curl_easy_setopt(curl, CURLOPT_URL, url.str().c_str());
	if (res != CURLE_OK) {
		throw curl_easy_strerror(res);
	}
	res = curl_easy_setopt(curl, CURLOPT_WRITEDATA, NULL);
	if (res != CURLE_OK) {
		throw curl_easy_strerror(res);
	}
	res = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
	if (res != CURLE_OK) {
		throw curl_easy_strerror(res);
	}
	res = curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
	if (res != CURLE_OK) {
		throw curl_easy_strerror(res);
	}
	res = curl_easy_perform(curl);
	if (res != CURLE_OK) {
		throw curl_easy_strerror(res);
	}
	if (res != CURLE_OK) {
		throw curl_easy_strerror(res);
	}
}

class Curl {
private:
	CURL *curl;
public:
	Curl(void) : curl(curl_easy_init()) {}
	virtual ~Curl() {
		if (curl) {
			curl_easy_cleanup(curl);
		}
	}
	operator CURL*(void) { return curl; }
	operator const CURL*(void) { return curl; }
};

void
connect_to_panels(const char *host_name, unsigned short port)
{
	Curl curl;
	do_connect_to_panels(curl, host_name, port);
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
            connect_to_panels(host_name, port);
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
	discover();
	curl_global_cleanup();
	return EXIT_SUCCESS;
}
