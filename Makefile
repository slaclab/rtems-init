
ifeq ($(RTEMS_TOP),)
$(error RTEMS_TOP must be defined before configuring)
endif

configure:
	./waf configure --prefix="$(RTEMS_TOP)/target/rtems" --rtems-tools="$(RTEMS_TOP)/host/linux-x86_64" --rtems "$(RTEMS_TOP)/target/rtems"
