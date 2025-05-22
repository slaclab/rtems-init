

ifeq ($(RTEMS_TOP),)
$(error RTEMS_TOP must be defined)
endif

# Determine available BSPs
BSPS=$(shell ls $(RTEMS_TOP)/target/rtems/lib/pkgconfig | cut -f 1 -d '.' | tr ' ' '\n')

ifndef T_A

########################################################
# Top-level rules, when T_A not defined
########################################################

all:
	$(foreach bsp,$(BSPS), \
		$(MAKE) -f Makefile.rtems T_A=$(bsp);)

clean:
	$(foreach bsp,$(BSPS), \
		$(MAKE) -f Makefile.rtems T_A=$(bsp) clean;)

install:
	$(foreach bsp,$(BSPS), \
		$(MAKE) -f Makefile.rtems T_A=$(bsp) install;)

.PHONY: install all clean

else

# Determine subdir based on T_A (target arch) setting
RTEMS_BSP=$(shell echo $(T_A) | cut -f 3 -d '-')
RTEMS_ARCH=$(shell echo $(T_A) | cut -f 1 -d '-')
RTEMS_VER=$(shell echo $(T_A) | cut -f 2 -d '-')
include $(RTEMS_TOP)/target/rtems/$(RTEMS_ARCH)-$(RTEMS_VER)/$(RTEMS_BSP)/Makefile.inc
include $(RTEMS_CUSTOM)

CFLAGS += $(CPU_CFLAGS) $(CPU_DEFINES) $(CFLAGS_OPTIMIZE_V)
CFLAGS += -Wno-strict-prototypes -Wno-missing-prototypes -DBSP_$(RTEMS_BSP)=1

# Network stack
ifeq ($(RTEMS_BSP),uC5282)
NETWORK_STACK=LEGACY
else
NETWORK_STACK=BSD
endif

# Important locations
PREFIX=$(RTEMS_ROOT)/$(RTEMS_TARGET)/$(RTEMS_BSP)
INCDIR=$(PREFIX)/lib/include
LIBDIR=$(PREFIX)/lib
BINDIR=$(PREFIX)/bin
SRCDIR=$(abspath .)
OBJDIR=build/$(T_A)

ifeq ($(NETWORK_STACK),BSD)
CFLAGS += -DRTEMS_LIBBSD_STACK=1
else
CFLAGS += -DRTEMS_LEGACY_STACK=1
endif

ifneq ($(RTEMS_BSP),uC5282)
HAVE_DEBUGGER=YES
endif

########################################################
# rtems-init
########################################################

RTEMS_INIT_SRCS = \
	src/getopt_s.c \
	src/hack.c \
	src/init.c \
	src/nvram.c \
	src/rtems-config.c \
	src/shell.c \
	src/util.c \
	src/net.c


ifeq ($(NETWORK_STACK),BSD)
RTEMS_INIT_SRCS += src/net_bsd.c
RTEMS_INIT_LIBS += -lbsd
else
RTEMS_INIT_SRCS += src/net_legacy.c
RTEMS_INIT_LIBS += -lnfs -lnetworking
endif

ifeq ($(HAVE_DEBUGGER),YES)
RTEMS_INIT_LIBS += -ldebugger
RTEMS_INIT_CFLAGS += -DHAVE_DEBUGGER=1
endif

RTEMS_INIT_OBJS:=$(addprefix $(OBJDIR)/rtems-init.dir/,$(RTEMS_INIT_SRCS:.c=.o) rootfs.o extra-syms.o)
RTEMS_INIT_LDFLAGS = -Wl,-u __symbolRefDummy -lrtemscxx -lrtemscpu -lrtemsbsp -lntp -ltelnetd -ltftpfs $(RTEMS_INIT_LIBS) -lc -lm
RTEMS_INIT_CFLAGS += $(CFLAGS)

$(OBJDIR)/rtems-init.dir/src/%.o: src/%.c
	mkdir -p $(OBJDIR)/rtems-init.dir/src
	$(CC) $(RTEMS_INIT_CFLAGS) -c -o $@ $^

# Extra sym ref generation
$(OBJDIR)/rtems-init.dir/extra-syms.o: | ./tools/sym/base.sym
	./tools/mksyms.py -o $(@:.o=.c) -r ./tools/sym/base.sym
	$(CC) $(RTEMS_INIT_CFLAGS) -c -o $@ $(@:.o=.c)

# Rootfs generation
$(OBJDIR)/rtems-init.dir/rootfs.o:
	./tools/mkrootfs.py -o $(@:.o=.S) -t -i rootfs -m "BSP_LIBS=" -m "TOOLCHAIN_LIBS="
	$(CC) $(RTEMS_INIT_CFLAGS) -c -o $@ $(@:.o=.S)

# Stage 1 link
$(OBJDIR)/rtems-init: $(RTEMS_INIT_OBJS)
	@echo "Linking stage 1 executable $@"
	$(CC) $(RTEMS_INIT_CFLAGS) -o $@ $^ $(LDFLAGS) $(RTEMS_INIT_LDFLAGS)

# Symbol generation for stage 2
$(OBJDIR)/rtems-init-syms.o: $(OBJDIR)/rtems-init
	rtems-syms -e -C $(firstword $(CC)) -o $@ $<

# Stage 2 link
$(OBJDIR)/rtems-init.exe: $(RTEMS_INIT_OBJS) $(OBJDIR)/rtems-init-syms.o
	@echo "Linking stage 2 executable $@"
	$(CC) $(RTEMS_INIT_CFLAGS) -o $@ $^ $(LDFLAGS) $(RTEMS_INIT_LDFLAGS)

# Bootfile generation
$(OBJDIR)/rtems-init.boot: $(OBJDIR)/rtems-init.exe
	@echo "Generating bootable image $@"
	$(OBJCOPY) -O binary $^ $@

rtems-init-install: $(OBJDIR)/rtems-init.boot
	mkdir -p $(BINDIR)
	install -m 777 $(OBJDIR)/rtems-init.boot $(BINDIR)/rtems-init.boot
	install -m 777 $(OBJDIR)/rtems-init.exe $(BINDIR)/rtems-init.exe
	
EXTRA_INSTALL_TARGETS+=rtems-init-install

########################################################
# libbspExt
########################################################
BSPEXT_SRCS = \
	bspExt/bspExt.c \
	bspExt/cacheUtil.c \
	bspExt/dabrBpnt.c \
	bspExt/isrWrap.c \
	bspExt/memProbe.c
	
BSPEXT_OBJS:=$(addprefix $(OBJDIR)/bspExt.dir/,$(BSPEXT_SRCS:.c=.o))

$(OBJDIR)/bspExt.dir/bspExt/%.o: bspExt/%.c
	mkdir -p $(OBJDIR)/bspExt.dir/bspExt
	$(CC) $(CFLAGS) -c -o $@ $^

# Object linking
$(OBJDIR)/bspExt.obj: $(BSPEXT_OBJS) | $(OBJDIR)/rtems-init.exe
	rtems-ld -O rap -e rtemsEntryPoint -o $@ -L$(RTEMS_ROOT)/$(RTEMS_TARGET)/$(RTEMS_BSP)/lib -b $(OBJDIR)/rtems-init.exe -lrtemsbsp -lrtemscpu $^

bspExt-install: $(OBJDIR)/bspExt.obj
	mkdir -p $(INCDIR)/bsp
	mkdir -p $(BINDIR)
	install -m 644 bspExt/bspExt.h $(INCDIR)/bsp/bspExt.h
	install -m 777 $(OBJDIR)/bspExt.obj $(BINDIR)/bspExt.obj

ifeq ($(RTEMS_ARCH),powerpc)
EXTRA_TARGETS+=$(OBJDIR)/bspExt.obj
EXTRA_INSTALL_TARGETS += bspExt-install
endif

########################################################
# Cexpsh
########################################################

$(SRCDIR)/cexpsh/configure:
	cd `dirname $@` && \
	autoreconf -i

# Derived from ssrlApps Makefile
$(OBJDIR)/cexpsh.dir/Makefile: $(SRCDIR)/cexpsh/configure
	mkdir -p `dirname $@`
	cd `dirname $@` && \
	pwd && \
	unset CC CFLAGS CXX CPPFLAGS LDFLAGS && \
	$(SRCDIR)/cexpsh/configure --host=$(RTEMS_ARCH)-$(RTEMS_VER) --disable-nls --prefix=$(PREFIX) --with-newlib --disable-multilib --with-rtems-top=$(RTEMS_ROOT) --enable-rtemsbsp=$(RTEMS_BSP)

cexpsh-makefile: $(OBJDIR)/cexpsh.dir/Makefile

########################################################
# Top-level rules
########################################################

all: $(OBJDIR)/rtems-init.boot $(EXTRA_TARGETS)

install: $(EXTRA_INSTALL_TARGETS)

clean:
	rm -rf $(OBJDIR)

# This is for testing
LOC=/sdf/group/cds/sw/epics/users/lorelli/rtems/6.1
copy: $(OBJDIR)/rtems-init.boot
	mkdir -p $(LOC)/target/rtems/$(RTEMS_ARCH)-$(RTEMS_VER)/$(RTEMS_BSP)/bin/
	cp -v $(OBJDIR)/rtems-init.boot $(LOC)/target/rtems/$(RTEMS_ARCH)-$(RTEMS_VER)/$(RTEMS_BSP)/bin/
	cp -v $(OBJDIR)/rtems-init.exe $(LOC)/target/rtems/$(RTEMS_ARCH)-$(RTEMS_VER)/$(RTEMS_BSP)/bin/

.PHONY: all clean install copy

endif

list-targets:
	@echo "Available targets:"
	@$(foreach bsp,$(BSPS), \
		echo "  $(bsp)";)
