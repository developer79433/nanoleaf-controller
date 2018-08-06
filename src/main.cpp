#include <iostream>
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
    AVAHI_GCC_UNUSED void* userdata) {
    assert(r);
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
                    "\tis_local: %i" << !!(flags & AVAHI_LOOKUP_RESULT_LOCAL) << std::endl <<
                    "\tour_own: %i" << !!(flags & AVAHI_LOOKUP_RESULT_OUR_OWN) << std::endl <<
                    "\twide_area: %i" << !!(flags & AVAHI_LOOKUP_RESULT_WIDE_AREA) << std::endl <<
                    "\tmulticast: %i" << !!(flags & AVAHI_LOOKUP_RESULT_MULTICAST) << std::endl <<
                    "\tcached: %i" << !!(flags & AVAHI_LOOKUP_RESULT_CACHED) << std::endl
			;
            avahi_free(t);
        }
    }
    avahi_service_resolver_free(r);
}

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
    AvahiClient *c = static_cast<AvahiClient *>(userdata);
    assert(b);
    /* Called whenever a new services becomes available on the LAN or is removed from the LAN */
    const char *errmsg;
    switch (event) {
        case AVAHI_BROWSER_FAILURE:
        	errmsg = avahi_strerror(avahi_client_errno(avahi_service_browser_get_client(b)));
            avahi_simple_poll_quit(simple_poll);
            throw errmsg;
        case AVAHI_BROWSER_NEW:
            fprintf(stderr, "(Browser) NEW: service '%s' of type '%s' in domain '%s'\n", name, type, domain);
            /* We ignore the returned resolver object. In the callback
               function we free it. If the server is terminated before
               the callback function is called the server will free
               the resolver for us. */
            if (!(avahi_service_resolver_new(c, interface, protocol, name, type, domain, AVAHI_PROTO_UNSPEC, AVAHI_LOOKUP_USE_MULTICAST, resolve_callback, c)))
                throw avahi_strerror(avahi_client_errno(c));
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
static void client_callback(AvahiClient *c, AvahiClientState state, AVAHI_GCC_UNUSED void * userdata) {
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
    AvahiClient *client = avahi_client_new(poll, AVAHI_CLIENT_NO_FAIL, client_callback, NULL, &error);
    if (!client) {
        throw avahi_strerror(error);
    }
    /* Create the service browser */
    AvahiServiceBrowser *sb = avahi_service_browser_new(
    	client,
		AVAHI_IF_UNSPEC,
		AVAHI_PROTO_UNSPEC,
		NANOLEAF_MDNS_SERVICE_TYPE,
		NULL,
		AVAHI_LOOKUP_USE_MULTICAST,
		browse_callback,
		client
	);
    if (!sb) {
        throw avahi_strerror(avahi_client_errno(client));
    }
    /* Run the main loop */
    avahi_simple_poll_loop(simple_poll);
}

int
main(int argc, char *argv[])
{
	discover();
	CURL *curl = curl_easy_init();
	curl_easy_setopt(curl, CURLOPT_URL, URL);
	curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
	CURLcode res = curl_easy_perform(curl);
	if (res != CURLE_OK) {
		fprintf(
			stderr,
			"curl_easy_perform() failed: %s\n",
			curl_easy_strerror(res)
		);
	}
	curl_easy_cleanup(curl);
	return EXIT_SUCCESS;
}
