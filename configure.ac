AC_INIT(nanoleaf_controller, 0.1, cnhfci3wngoifuxq3nbkuxqn3guykfgxwunwfk4@mailinator.com)
AC_LANG(C++)
AC_PROG_CXX()
LIBS=$(curl-config --libs)
CXXFLAGS=" -pedantic -Wall -Werror -std=gnu++14 -g3 $(curl-config --cflags)"
LDFLAGS+="$LIBS"
AC_CONFIG_SRCDIR([src/main.cpp])
m4_include([m4/check_cpp_lib.m4])
AC_PREFIX_DEFAULT(/)
AM_INIT_AUTOMAKE
AC_CONFIG_HEADERS([include/config.h])
AH_TOP([#ifndef CONFIG_H
#define CONFIG_H 1])
AH_BOTTOM([#endif /* CONFIG_H */])
dnl
dnl Check for CurlPP header
dnl
dnl AC_CHECK_HEADER([curlpp/Easy.hpp], [
dnl 	AC_DEFINE([HAVE_CURLPP_EASY_H], [1], [Define if you have curlpp/Easy.hpp])
dnl ], [
dnl 	AC_MSG_ERROR([Cannot find curlpp/Easy.hpp])
dnl ])
dnl
dnl Check for CurlPP usability
dnl
dnl CHECK_CPP_LIB([curlpp], [curlpp::initialize], [
dnl #include <curlpp/cURLpp.hpp>
dnl ])
AC_CHECK_LIB([curl], [curl_easy_init])
AC_CONFIG_FILES([Makefile src/Makefile man/Makefile])
AC_OUTPUT
