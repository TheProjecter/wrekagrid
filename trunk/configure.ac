AC_INIT([Wrekavoc],[0.5],[olivier.dubuisson@loria.fr])
AM_INIT_AUTOMAKE([-Wall])
AC_PROG_CC

AC_DEFINE(erreur,0)

PKG_CHECK_MODULES(LIBXML, xml2,erreur=0,erreur=1)
if test $erreur = 1; then
	PKG_CHECK_MODULES(LIBXML, libxml-2.0)
fi
AC_SUBST(LIBXML_CFLAGS)
AC_SUBST(LIBXML_LIBS)

PKG_CHECK_MODULES(LIBGSL, gsl)
AC_SUBST(LIBGSL_CFLAGS)
AC_SUBST(LIBGSL_LIBS)

AC_CHECK_LIB(m, hypot, [MATH_LIBS=-lm
 AC_SUBST(MATH_LIBS)], AC_MSG_ERROR([*** libm is required]))

AC_CHECK_LIB(pthread, pthread_create, [PTHREADS_LIBS=-pthread
 AC_SUBST(PTHREADS_LIBS)], AC_MSG_ERROR([*** lib pthreads is required]))

AC_CONFIG_FILES([
	Makefile 
	src/Makefile
	sample/Makefile
	sample/src/Makefile
	sample/mpi/Makefile
	sample/mpi/src/Makefile
])
AC_OUTPUT
