LIBS=-lcurlpp
AM_CFLAGS=-pedantic -Wall -Werror -std=gnu++14 -g3 $(curlpp-config --cflags)
AM_LDFLAGS=$(curlpp-config --libs)
bin_PROGRAMS = nanoleaf_controller
nanoleaf_controller_SOURCES = main.cpp
