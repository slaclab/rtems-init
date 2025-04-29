AUTOMAKE_OPTIONS=foreign

ACLOCAL_AMFLAGS= -I./m4

# make sure any third-party makefile
# inherits these settings which
# may be embedded in installation paths...
# (This is some hack)
export rtems_bsp
export RTEMS_BSP
export enable_rtemsbsp
export package_subdir

SUBDIRS=. @enable_subdirs@

DIST_SUBDIRS= @all_subdirs@

EXTRA_DIST=makefile.top.am makefile.top.in ssrlApps.components.in dummycxx bootstrap
EXTRA_DIST+=m4/ChangeLog

#Remove recursion introduced by default
#install-am: all-am
install-am:

all-local:
	$(MAKE) $(AM_MAKEFLAGS) INSTALL="$(INSTALL_IF_CHANGE)" prefix="$(abs_top_builddir)/data" exec_prefix='$$(prefix)' includedir='$$(prefix)/include' install

clean-local:
	$(RM) -r data