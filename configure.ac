AC_INIT(nanoleaf_controller, 0.1, cnhfci3wngoifuxq3nbkuxqn3guykfgxwunwfk4@mailinator.com)
AC_LANG(C++)
AC_PROG_CXX()
AC_CONFIG_SRCDIR([src/main.cpp])
m4_include([m4/check_cpp_lib.m4])
AC_PREFIX_DEFAULT(/)
AM_INIT_AUTOMAKE
AC_CONFIG_HEADERS([include/config.h])
AH_TOP([#ifndef CONFIG_H
#define CONFIG_H 1])
AH_BOTTOM([#endif /* CONFIG_H */])
AC_CHECK_HEADER([curlpp/Easy.hpp], [
	AC_DEFINE([HAVE_CURLPP_EASY_H], [1], [Define if you have curlpp/Easy.hpp])
], [
	AC_MSG_ERROR([Cannot find curlpp/Easy.hpp])
])
CHECK_CPP_LIB([curlpp], [curlpp::initialize], [
#include <curlpp/cURLpp.hpp>
])
AC_CONFIG_FILES([Makefile src/Makefile man/Makefile])
AC_OUTPUT
