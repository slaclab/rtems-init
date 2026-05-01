/**
 * ----------------------------------------------------------------------------
 * Company    : SLAC National Accelerator Laboratory
 * ----------------------------------------------------------------------------
 * Description: Legacy networking stack init code
 * ----------------------------------------------------------------------------
 * This file is part of 'rtems-init'. It is subject to the license terms in the
 * LICENSE.txt file found in the top-level directory of this distribution,
 * and at:
 *    https://confluence.slac.stanford.edu/display/ppareg/LICENSE.html.
 * No part of 'rtems-init', including this file, may be copied, modified,
 * propagated, or distributed except according to the terms contained in the
 * LICENSE.txt file.
 * ----------------------------------------------------------------------------
 **/
#include <rtems.h>
#include <bsp.h>
#include <rtems/rtems_bsdnet.h>
#include <rtems/pci.h>
#include <rtems/error.h>
#include <rtems/telnetd.h>
#include <rtems/dhcp.h>

#include <stdint.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

#include "rtems-init.h"

/****************************************************************************\
 * ne2kpci hackery.
 * This code was pulled from EPICS base as a workaround for testing inside
 * QEMU, specifically on i386 BSPs
\****************************************************************************/

#ifdef __i386__
int
rtems_ne2kpci_driver_attach (struct rtems_bsdnet_ifconfig *config, int attach)
{
  uint8_t  irq;
  uint32_t bar0;
  int B, D, F, ret;
  klog("ne2kpci: Probing for NE2000 on PCI (aka. Realtek 8029)\n");

  if(pci_find_device(PCI_VENDOR_ID_REALTEK, PCI_DEVICE_ID_REALTEK_8029, 0, &B, &D, &F))
  {
    klog("ne2kpci: Not found\n");
    return 0;
  }

  klog("ne2kpci: Found %d:%d.%d\n", B, D, F);

  ret = pci_read_config_dword(B, D, F, PCI_BASE_ADDRESS_0, &bar0);
  ret|= pci_read_config_byte(B, D, F, PCI_INTERRUPT_LINE, &irq);

  if(ret || (bar0&PCI_BASE_ADDRESS_SPACE)!=PCI_BASE_ADDRESS_SPACE_IO)
  {
    kerror("ne2kpci: Failed reading card config\n");
    return 0;
  }

  config->irno = irq;
  config->port = bar0&PCI_BASE_ADDRESS_IO_MASK;

  klog("ne2kpci: Using port=0x%x irq=%u\n", (unsigned)config->port, config->irno);

  return rtems_ne_driver_attach(config, attach);
}
#endif

static struct rtems_bsdnet_ifconfig loopback_config = {
  RTEMS_BSP_NETWORK_DRIVER_NAME,
  RTEMS_BSP_NETWORK_DRIVER_ATTACH,
};

#ifdef __i386__
static struct rtems_bsdnet_ifconfig ne2k_driver_config = {
  "ne1",
  (void*)&rtems_ne2kpci_driver_attach,
  &loopback_config,
  "10.0.2.15",
  "255.255.255.0",
};
#endif

struct rtems_bsdnet_config rtems_bsdnet_config = {
  .network_task_priority = 10,
  .bootp = rtems_bsdnet_do_bootp_and_rootfs,
  .mbuf_bytecount = RTEMS_NETWORK_CONFIG_MBUF_SPACE * 1024,
  .mbuf_cluster_bytecount = RTEMS_NETWORK_CONFIG_CLUSTER_SPACE * 1024,
  .domainname = "slac.stanford.edu",
#ifdef __i386__
  .ifconfig = &ne2k_driver_config,
#else
  .ifconfig = &loopback_config,
#endif
};

/****************************************************************************\
 * Network init
\****************************************************************************/

void
network_init()
{
  klog("Starting legacy network stack\n");

  /* dhcpcd will fill this out */
  //creat("/etc/resolv.conf", 0666);

  rtems_bsdnet_initialize_network();
  rtems_bsdnet_show_if_stats();

  klog("Starting ntpd\n");

  if (ntp_init() != 0) {
    kerror("NTP init failed; it will now be disabled\n");
  }

  klog("Starting telnetd\n");
  if (rtems_telnetd_initialize() != RTEMS_SUCCESSFUL) {
    kerror("Failed to init telnetd\n");
  }

  klog("End legacy network init\n");
}
