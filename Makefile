
ifeq ($(RTEMS_TOP),)
$(error RTEMS_TOP must be defined before configuring)
endif

TARGETS?=$(shell ls $(RTEMS_TOP)/target/rtems/lib/pkgconfig/ | grep -Po "(?<=(powerpc|arm|i386|m68k)-).*" | cut -d '.' -f 1 | tr '\n' ' ')

TARGETS+=rtems6-pc686-qemu

$(info TARGETS=$(TARGETS))

ifneq ($(PREFIX),)
EXTRA_ARGS=-DCMAKE_INSTALL_PREFIX="$(PREFIX)" -DSHARED_PREFIX=ON
endif

configure:
	./waf configure --prefix="$(RTEMS_TOP)/target/rtems" --rtems-tools="$(RTEMS_TOP)/host/linux-x86_64" --rtems "$(RTEMS_TOP)/target/rtems"

cmake-configure:
	$(foreach target,$(TARGETS),cmake -Bbuild-cmake/build-$(target) -GNinja -DRTEMS_TOP=$(RTEMS_TOP) \
		-DCMAKE_TOOLCHAIN_FILE=rtems-tools/toolchains/$(target).cmake $(EXTRA_ARGS);)

build:
	$(foreach target,$(TARGETS),ninja -C build-cmake/build-$(target);)

clean:
	$(foreach target,$(TARGETS),ninja -C build-cmake/build-$(target) clean;)

build-clean:
	$(foreach target,$(TARGETS),rm -rf build-cmake/build-$(target);)

install:
	$(foreach target,$(TARGETS),ninja -C build-cmake/build-$(target) install;)


.PHONY: build clean install configure cmake-configure build-clean
