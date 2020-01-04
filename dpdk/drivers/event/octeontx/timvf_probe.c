/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright(c) 2017 Cavium, Inc
 */

#include <rte_eal.h>
#include <rte_io.h>
#include <rte_pci.h>
#include <rte_bus_pci.h>

#include <octeontx_mbox.h>

#include "ssovf_evdev.h"
#include "timvf_evdev.h"

#ifndef PCI_VENDOR_ID_CAVIUM
#define PCI_VENDOR_ID_CAVIUM			(0x177D)
#endif

#define PCI_DEVICE_ID_OCTEONTX_TIM_VF		(0xA051)
#define TIM_MAX_RINGS				(64)

struct timvf_res {
	uint16_t domain;
	uint16_t vfid;
	void *bar0;
	void *bar2;
	void *bar4;
};

struct timdev {
	uint8_t total_timvfs;
	struct timvf_res rings[TIM_MAX_RINGS];
};

static struct timdev tdev;

int
timvf_info(struct timvf_info *tinfo)
{
	int i;
	struct ssovf_info info;

	if (tinfo == NULL)
		return -EINVAL;

	if (!tdev.total_timvfs)
		return -ENODEV;

	if (ssovf_info(&info) < 0)
		return -EINVAL;

	for (i = 0; i < tdev.total_timvfs; i++) {
		if (info.domain != tdev.rings[i].domain) {
			timvf_log_err("GRP error, vfid=%d/%d domain=%d/%d %p",
				i, tdev.rings[i].vfid,
				info.domain, tdev.rings[i].domain,
				tdev.rings[i].bar0);
			return -EINVAL;
		}
	}

	tinfo->total_timvfs = tdev.total_timvfs;
	tinfo->domain = info.domain;
	return 0;
}

void*
timvf_bar(uint8_t id, uint8_t bar)
{
	if (rte_eal_process_type() != RTE_PROC_PRIMARY)
		return NULL;

	if (id > tdev.total_timvfs)
		return NULL;

	switch (bar) {
	case 0:
		return tdev.rings[id].bar0;
	case 4:
		return tdev.rings[id].bar4;
	default:
		return NULL;
	}
}

static int
timvf_probe(struct rte_pci_driver *pci_drv, struct rte_pci_device *pci_dev)
{
	uint64_t val;
	uint16_t vfid;
	struct timvf_res *res;

	RTE_SET_USED(pci_drv);

	/* For secondary processes, the primary has done all the work */
	if (rte_eal_process_type() != RTE_PROC_PRIMARY)
		return 0;

	if (pci_dev->mem_resource[0].addr == NULL ||
			pci_dev->mem_resource[4].addr == NULL) {
		timvf_log_err("Empty bars %p %p",
				pci_dev->mem_resource[0].addr,
				pci_dev->mem_resource[4].addr);
		return -ENODEV;
	}

	val = rte_read64((uint8_t *)pci_dev->mem_resource[0].addr +
			0x100 /* TIM_VRINGX_BASE */);
	vfid = (val >> 23) & 0xff;
	if (vfid >= TIM_MAX_RINGS) {
		timvf_log_err("Invalid vfid(%d/%d)", vfid, TIM_MAX_RINGS);
		return -EINVAL;
	}

	res = &tdev.rings[tdev.total_timvfs];
	res->vfid = vfid;
	res->bar0 = pci_dev->mem_resource[0].addr;
	res->bar2 = pci_dev->mem_resource[2].addr;
	res->bar4 = pci_dev->mem_resource[4].addr;
	res->domain = (val >> 7) & 0xffff;
	tdev.total_timvfs++;
	rte_wmb();

	timvf_log_dbg("Domain=%d VFid=%d bar0 %p total_timvfs=%d", res->domain,
			res->vfid, pci_dev->mem_resource[0].addr,
			tdev.total_timvfs);
	return 0;
}


static const struct rte_pci_id pci_timvf_map[] = {
	{
		RTE_PCI_DEVICE(PCI_VENDOR_ID_CAVIUM,
				PCI_DEVICE_ID_OCTEONTX_TIM_VF)
	},
	{
		.vendor_id = 0,
	},
};

static struct rte_pci_driver pci_timvf = {
	.id_table = pci_timvf_map,
	.drv_flags = RTE_PCI_DRV_NEED_MAPPING | RTE_PCI_DRV_IOVA_AS_VA,
	.probe = timvf_probe,
	.remove = NULL,
};

RTE_PMD_REGISTER_PCI(octeontx_timvf, pci_timvf);
