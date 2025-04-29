AC_PREREQ(2.70)

m4_include(./m4/cvstag.m4)

AC_INIT(rtems-init, TILLAC_GITTAG, <lorelli@slac.stanford.edu>)

TILLAC_CHECK_GITTAG

AC_CANONICAL_BUILD
AC_CANONICAL_HOST
AC_CANONICAL_TARGET

AM_INIT_AUTOMAKE([foreign 1.10])
AM_MAINTAINER_MODE

TILLAC_RTEMS_CHECK_TOOLS

TILLAC_RTEMS_SETUP

AC_SUBST([rtems_cpu],[$host_cpu])
AC_SUBST([rtems_bsp],[$enable_rtemsbsp])

AC_CONFIG_FILES([Makefile])

AC_CONFIG_SUBDIRS([cexpsh])
AC_DEFINE(HAVE_CEXP,1,[Whether CEXP is installed])

AC_SUBST([system_subdir])

AM_CONDITIONAL([HAVE_CEXP], [test "$have_cexp" = "yes"])

AC_OUTPUT