#ifndef DISCOVERY_H
#define DISCOVERY_H 1

#include <iostream>
#include <string>
#include <avahi-client/client.h>
#include <avahi-client/lookup.h>
#include <avahi-common/simple-watch.h>
#include <avahi-common/malloc.h>
#include <avahi-common/error.h>

class MDNSResponder {
private:
    typedef bool (*host_callback_t)(const AvahiAddress &address, const std::string &hostname, uint16_t port, const std::string &id, void *userdata);
    class Session {
    private:
    	MDNSResponder *responder;
        host_callback_t host_callback;
        void *callback_arg;
        const std::string service_type;
    	AvahiSimplePoll *simple_poll;
    	AvahiClient *client;
	private:
		/**
		 * These are static so they can be accessed from the resolve callback,
		 * potentially from multiple threads concurrently, so access to them
		 * must be serialised using a mutex.
		 */
        static host_callback_t shared_host_callback;
        static void *shared_callback_arg;
        static bool shared_continue_enumeration;
    private:
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
			void *userdata
		);
		void new_device(
			AvahiIfIndex interface,
			AvahiProtocol protocol,
			const char *name,
			const char *type,
			const char *domain
		);
    	void browse_callback(
    	    AvahiServiceBrowser *b,
    	    AvahiIfIndex interface,
    	    AvahiProtocol protocol,
    	    AvahiBrowserEvent event,
    	    const char *name,
    	    const char *type,
    	    const char *domain,
    	    AVAHI_GCC_UNUSED AvahiLookupResultFlags flags
    	);
    	static void client_callback(AvahiClient *c, AvahiClientState state, void *userdata) {
    		Session *tthis = static_cast<Session *>(userdata);
    		tthis->client_callback(c, state);
    	}
    	void client_callback(AvahiClient *c, AvahiClientState state) {
    	    assert(c);
    	    /* Called whenever the client or server state changes */
    	    if (state == AVAHI_CLIENT_FAILURE) {
    	        const char *errmsg = avahi_strerror(avahi_client_errno(c));
    	        avahi_simple_poll_quit(simple_poll);
    	        throw errmsg;
    	    }
    	}
    public:
    	static void browse_callback(
    	    AvahiServiceBrowser *b,
    	    AvahiIfIndex interface,
    	    AvahiProtocol protocol,
    	    AvahiBrowserEvent event,
    	    const char *name,
    	    const char *type,
    	    const char *domain,
    	    AVAHI_GCC_UNUSED AvahiLookupResultFlags flags,
    	    void* userdata
    	) {
    		Session *tthis = static_cast<Session *>(userdata);
    		tthis->browse_callback(b, interface, protocol, event, name, type, domain, flags);
    	}
    	void discover() {
    	    AvahiServiceBrowser *sb = avahi_service_browser_new(
    	    	client,
    			AVAHI_IF_UNSPEC,
    			AVAHI_PROTO_UNSPEC,
    			service_type.c_str(),
    			NULL,
    			AVAHI_LOOKUP_USE_MULTICAST,
    			Session::browse_callback,
    			this
    		);
    	    if (!sb) {
    	        throw avahi_strerror(avahi_client_errno(client));
    	    }
    	    /* Run the main loop */
    	    avahi_simple_poll_loop(simple_poll);
    	}
        Session(
        	MDNSResponder *presponder,
			host_callback_t phost_callback,
			void *pcallback_arg,
			const std::string &pservice_type
		) :
			responder(presponder),
			host_callback(phost_callback),
			callback_arg(pcallback_arg),
			service_type(pservice_type)
    	{
    	    /* Allocate main loop object */
    	    if (!(simple_poll = avahi_simple_poll_new())) {
    	        throw "avahi_simple_poll_new failed";
    	    }
    	    const AvahiPoll *poll = avahi_simple_poll_get(simple_poll);
    		int error;
    	    client = avahi_client_new(poll, AVAHI_CLIENT_NO_FAIL, client_callback, this, &error);
    	    if (!client) {
    	        throw avahi_strerror(error);
    	    }
        }
        ~Session() {}
    };
private:
public:
	MDNSResponder() {
	}
	virtual ~MDNSResponder() {}
	void discover(const std::string &service_type, host_callback_t callback, void *callback_arg) {
		Session session(this, callback, callback_arg, service_type);
		session.discover();
	}
};

#endif /* DISCOVERY_H */
