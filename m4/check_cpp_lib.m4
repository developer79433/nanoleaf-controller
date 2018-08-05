AC_DEFUN([TO_MACRO_NAME],[[HAVE_]translit(translit(translit($1,[a-z],[A-Z]),[.],[_]),[:],[_])])
AC_DEFUN([CHECK_LIB_FUNC_TO_MACRO_COMMENT],[Define to 1 if lib]$1[ exports a function ]$2)
AC_DEFUN([CHECK_CPP_LIB], [
AC_MSG_CHECKING(
[whether lib$1 contains $2]
)
AC_LANG(C++)
SAVED_LDFLAGS=$LDFLAGS
LDFLAGS="$LDFLAGS -l$1"
AC_LINK_IFELSE(
	[
		AC_LANG_PROGRAM($3, [
void (*func_ptr);
func_ptr = (void (*)) $2;
		])
	],
	[TEST_LIBS="$TEST_LIBS -l$1"]
	[
		AC_MSG_RESULT([yes])
		AC_DEFINE(TO_MACRO_NAME($2), [1], CHECK_LIB_FUNC_TO_MACRO_COMMENT($1, $2))
	],
	[
		AC_MSG_RESULT([no])
		AC_DEFINE(TO_MACRO_NAME($2), [0], CHECK_LIB_FUNC_TO_MACRO_COMMENT($1, $2))
	]
)
LDFLAGS=$SAVED_LDFLAGS
])
