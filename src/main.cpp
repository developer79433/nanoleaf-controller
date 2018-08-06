#include <iostream>
#include <cstdlib>
#include <curl/curl.h>
#include <curl/easy.h>

#define URL "http://192.168.1.17/"

int
main(int argc, char *argv[])
{
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
