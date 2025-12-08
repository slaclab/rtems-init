.. rtems-init documentation master file, created by
   sphinx-quickstart on Fri Nov 14 11:34:18 2025.
   You can adapt this file completely to your liking, but it should at least
   contain the root `toctree` directive.

rtems-init
==========

rtems-init is an init system for RTEMS 6+ and a set of modules for RTEMS applications, particularly EPICS IOCs.
rtems-init was designed to replace ssrlApps and the GeSys components developed for RTEMS 4.X at SLAC.

This documentation will go over the implementation of rtems-init, supported hardware, build facility,
steps for doing a release and more.

Features
--------

* Support for RTEMS 6+
* BSD or legacy networking
   * BSD networking includes NFSv2/3/4 plus more advanced DHCP configuration
* NTP (via rtems-net-services)
* DHCP or NVRAM based configuration
   * Configures DNS, NTP, automatic NFS/9P mounts
* System level Lua scripting
* rootfs system
* Cexpsh shell support
* Modern CMake build system
* Hardware and emulated testing support
* Additional on-target command line utilities

Contents
--------
.. toctree::
   :maxdepth: 2

   supported-hardware.md
