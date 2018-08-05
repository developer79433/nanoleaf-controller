#include <iostream>
#include <cstdlib>
#include <curlpp/cURLpp.hpp>

int
main(int argc, char *argv[])
{
	curlpp::initialize();
	curlpp::terminate();
	return EXIT_SUCCESS;
}
