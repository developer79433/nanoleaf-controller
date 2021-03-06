#include "discovery.h"

MDNSResponder::host_callback_t MDNSResponder::Session::shared_host_callback = NULL;
void *MDNSResponder::Session::shared_callback_arg = NULL;
bool MDNSResponder::Session::shared_continue_enumeration = true;

void MDNSResponder::Session::resolve_callback(
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
	// In my development system, this parameter receives apparently-garbage
	// values; it does not (as one might expect) receive the value passed
	// to avahi_service_resolver_new.
	// assert(userdata);
    assert(r);
    /* Called whenever a service has been resolved successfully or timed out */
    switch (event) {
	case AVAHI_RESOLVER_FAILURE:
		throw avahi_strerror(avahi_client_errno(avahi_service_resolver_get_client(r)));
	case AVAHI_RESOLVER_FOUND:
		{
			char a[AVAHI_ADDRESS_STR_MAX], *t;
			std::cerr << "Service '" << name << "' of type '" << type << "' in domain '" << domain << "':" << std::endl;
			avahi_address_snprint(a, sizeof(a), address);
			t = avahi_string_list_to_string(txt);
			std::string id;
			for (AvahiStringList *l = txt; l != NULL; l = avahi_string_list_get_next(l)) {
				if (l->size > 3 && 'i' == l->text[0] && 'd' == l->text[1] && '=' == l->text[2]) {
					id = reinterpret_cast<char *>(&(l->text[3]));
				}
			}
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
			shared_continue_enumeration = shared_host_callback(
				*address, host_name, port, id, shared_callback_arg
			);
			avahi_free(t);
		}
		break;
	default:
		break;
    }
    avahi_service_resolver_free(r);
}

void MDNSResponder::Session::new_device(
	AvahiIfIndex interface,
	AvahiProtocol protocol,
	const char *name,
	const char *type,
	const char *domain
) {
	// TODO: Begin critical section
	shared_host_callback = host_callback;
	shared_callback_arg = callback_arg;
	AvahiServiceResolver *ret = avahi_service_resolver_new(
		client,
		interface,
		protocol,
		name,
		type,
		domain,
		AVAHI_PROTO_UNSPEC,
		AVAHI_LOOKUP_USE_MULTICAST,
		resolve_callback,
		// In my development system, this parameter appears to be
		// ignored; it does not (as one might expect) get passed
		// to resolve_callback.
		NULL
	);
	bool continue_enumeration = shared_continue_enumeration;
	// TODO: End critical section
	if (!ret) {
		throw avahi_strerror(avahi_client_errno(client));
	}
    if (!continue_enumeration) {
		avahi_simple_poll_quit(simple_poll);
    }
}

void MDNSResponder::Session::browse_callback(
    AvahiServiceBrowser *b,
    AvahiIfIndex interface,
    AvahiProtocol protocol,
    AvahiBrowserEvent event,
    const char *name,
    const char *type,
    const char *domain,
    AVAHI_GCC_UNUSED AvahiLookupResultFlags flags
) {
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
		new_device(interface, protocol, name, type, domain);
		break;
	case AVAHI_BROWSER_REMOVE:
#ifndef NDEBUG
		std::cerr << "(Browser) REMOVE: service '" << name << "' of type '" << type << "' in domain '" << domain << "'" << std::endl;
#endif /* ndef NDEBUG */
		break;
	case AVAHI_BROWSER_ALL_FOR_NOW:
#ifndef NDEBUG
		std::cerr << "(Browser) ALL_FOR_NOW" << std::endl;
#endif /* ndef NDEBUG */
		avahi_simple_poll_quit(simple_poll);
		break;
	case AVAHI_BROWSER_CACHE_EXHAUSTED:
#ifndef NDEBUG
		std::cerr << "(Browser) CACHE_EXHAUSTED" << std::endl;
#endif /* ndef NDEBUG */
		break;
	default:
		break;
    }
}
