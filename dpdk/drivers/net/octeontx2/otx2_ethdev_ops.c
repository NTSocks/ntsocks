/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright(C) 2019 Marvell International Ltd.
 */

#include <rte_mbuf_pool_ops.h>

#include "otx2_ethdev.h"

int
otx2_nix_mtu_set(struct rte_eth_dev *eth_dev, uint16_t mtu)
{
	uint32_t buffsz, frame_size = mtu + NIX_L2_OVERHEAD;
	struct otx2_eth_dev *dev = otx2_eth_pmd_priv(eth_dev);
	struct rte_eth_dev_data *data = eth_dev->data;
	struct otx2_mbox *mbox = dev->mbox;
	struct nix_frs_cfg *req;
	int rc;

	/* Check if MTU is within the allowed range */
	if (frame_size < NIX_MIN_FRS || frame_size > NIX_MAX_FRS)
		return -EINVAL;

	buffsz = data->min_rx_buf_size - RTE_PKTMBUF_HEADROOM;

	/* Refuse MTU that requires the support of scattered packets
	 * when this feature has not been enabled before.
	 */
	if (data->dev_started && frame_size > buffsz &&
	    !(dev->rx_offloads & DEV_RX_OFFLOAD_SCATTER))
		return -EINVAL;

	/* Check <seg size> * <max_seg>  >= max_frame */
	if ((dev->rx_offloads & DEV_RX_OFFLOAD_SCATTER)	&&
	    (frame_size > buffsz * NIX_RX_NB_SEG_MAX))
		return -EINVAL;

	req = otx2_mbox_alloc_msg_nix_set_hw_frs(mbox);
	req->update_smq = true;
	/* FRS HW config should exclude FCS but include NPC VTAG insert size */
	req->maxlen = frame_size - RTE_ETHER_CRC_LEN + NIX_MAX_VTAG_ACT_SIZE;

	rc = otx2_mbox_process(mbox);
	if (rc)
		return rc;

	/* Now just update Rx MAXLEN */
	req = otx2_mbox_alloc_msg_nix_set_hw_frs(mbox);
	req->maxlen = frame_size - RTE_ETHER_CRC_LEN;

	rc = otx2_mbox_process(mbox);
	if (rc)
		return rc;

	if (frame_size > RTE_ETHER_MAX_LEN)
		dev->rx_offloads |= DEV_RX_OFFLOAD_JUMBO_FRAME;
	else
		dev->rx_offloads &= ~DEV_RX_OFFLOAD_JUMBO_FRAME;

	/* Update max_rx_pkt_len */
	data->dev_conf.rxmode.max_rx_pkt_len = frame_size;

	return rc;
}

int
otx2_nix_recalc_mtu(struct rte_eth_dev *eth_dev)
{
	struct otx2_eth_dev *dev = otx2_eth_pmd_priv(eth_dev);
	struct rte_eth_dev_data *data = eth_dev->data;
	struct rte_pktmbuf_pool_private *mbp_priv;
	struct otx2_eth_rxq *rxq;
	uint32_t buffsz;
	uint16_t mtu;
	int rc;

	/* Get rx buffer size */
	rxq = data->rx_queues[0];
	mbp_priv = rte_mempool_get_priv(rxq->pool);
	buffsz = mbp_priv->mbuf_data_room_size - RTE_PKTMBUF_HEADROOM;

	/* Setup scatter mode if needed by jumbo */
	if (data->dev_conf.rxmode.max_rx_pkt_len > buffsz)
		dev->rx_offloads |= DEV_RX_OFFLOAD_SCATTER;

	/* Setup MTU based on max_rx_pkt_len */
	mtu = data->dev_conf.rxmode.max_rx_pkt_len - NIX_L2_OVERHEAD;

	rc = otx2_nix_mtu_set(eth_dev, mtu);
	if (rc)
		otx2_err("Failed to set default MTU size %d", rc);

	return rc;
}

static void
nix_cgx_promisc_config(struct rte_eth_dev *eth_dev, int en)
{
	struct otx2_eth_dev *dev = otx2_eth_pmd_priv(eth_dev);
	struct otx2_mbox *mbox = dev->mbox;

	if (otx2_dev_is_vf(dev))
		return;

	if (en)
		otx2_mbox_alloc_msg_cgx_promisc_enable(mbox);
	else
		otx2_mbox_alloc_msg_cgx_promisc_disable(mbox);

	otx2_mbox_process(mbox);
}

void
otx2_nix_promisc_config(struct rte_eth_dev *eth_dev, int en)
{
	struct otx2_eth_dev *dev = otx2_eth_pmd_priv(eth_dev);
	struct otx2_mbox *mbox = dev->mbox;
	struct nix_rx_mode *req;

	if (otx2_dev_is_vf(dev))
		return;

	req = otx2_mbox_alloc_msg_nix_set_rx_mode(mbox);

	if (en)
		req->mode = NIX_RX_MODE_UCAST | NIX_RX_MODE_PROMISC;

	otx2_mbox_process(mbox);
	eth_dev->data->promiscuous = en;
	otx2_nix_vlan_update_promisc(eth_dev, en);
}

void
otx2_nix_promisc_enable(struct rte_eth_dev *eth_dev)
{
	otx2_nix_promisc_config(eth_dev, 1);
	nix_cgx_promisc_config(eth_dev, 1);
}

void
otx2_nix_promisc_disable(struct rte_eth_dev *eth_dev)
{
	otx2_nix_promisc_config(eth_dev, 0);
	nix_cgx_promisc_config(eth_dev, 0);
}

static void
nix_allmulticast_config(struct rte_eth_dev *eth_dev, int en)
{
	struct otx2_eth_dev *dev = otx2_eth_pmd_priv(eth_dev);
	struct otx2_mbox *mbox = dev->mbox;
	struct nix_rx_mode *req;

	if (otx2_dev_is_vf(dev))
		return;

	req = otx2_mbox_alloc_msg_nix_set_rx_mode(mbox);

	if (en)
		req->mode = NIX_RX_MODE_UCAST | NIX_RX_MODE_ALLMULTI;
	else if (eth_dev->data->promiscuous)
		req->mode = NIX_RX_MODE_UCAST | NIX_RX_MODE_PROMISC;

	otx2_mbox_process(mbox);
}

void
otx2_nix_allmulticast_enable(struct rte_eth_dev *eth_dev)
{
	nix_allmulticast_config(eth_dev, 1);
}

void
otx2_nix_allmulticast_disable(struct rte_eth_dev *eth_dev)
{
	nix_allmulticast_config(eth_dev, 0);
}

void
otx2_nix_rxq_info_get(struct rte_eth_dev *eth_dev, uint16_t queue_id,
		      struct rte_eth_rxq_info *qinfo)
{
	struct otx2_eth_rxq *rxq;

	rxq = eth_dev->data->rx_queues[queue_id];

	qinfo->mp = rxq->pool;
	qinfo->scattered_rx = eth_dev->data->scattered_rx;
	qinfo->nb_desc = rxq->qconf.nb_desc;

	qinfo->conf.rx_free_thresh = 0;
	qinfo->conf.rx_drop_en = 0;
	qinfo->conf.rx_deferred_start = 0;
	qinfo->conf.offloads = rxq->offloads;
}

void
otx2_nix_txq_info_get(struct rte_eth_dev *eth_dev, uint16_t queue_id,
		      struct rte_eth_txq_info *qinfo)
{
	struct otx2_eth_txq *txq;

	txq = eth_dev->data->tx_queues[queue_id];

	qinfo->nb_desc = txq->qconf.nb_desc;

	qinfo->conf.tx_thresh.pthresh = 0;
	qinfo->conf.tx_thresh.hthresh = 0;
	qinfo->conf.tx_thresh.wthresh = 0;

	qinfo->conf.tx_free_thresh = 0;
	qinfo->conf.tx_rs_thresh = 0;
	qinfo->conf.offloads = txq->offloads;
	qinfo->conf.tx_deferred_start = 0;
}

static void
nix_rx_head_tail_get(struct otx2_eth_dev *dev,
		     uint32_t *head, uint32_t *tail, uint16_t queue_idx)
{
	uint64_t reg, val;

	if (head == NULL || tail == NULL)
		return;

	reg = (((uint64_t)queue_idx) << 32);
	val = otx2_atomic64_add_nosync(reg, (int64_t *)
				       (dev->base + NIX_LF_CQ_OP_STATUS));
	if (val & (OP_ERR | CQ_ERR))
		val = 0;

	*tail = (uint32_t)(val & 0xFFFFF);
	*head = (uint32_t)((val >> 20) & 0xFFFFF);
}

uint32_t
otx2_nix_rx_queue_count(struct rte_eth_dev *eth_dev, uint16_t queue_idx)
{
	struct otx2_eth_rxq *rxq = eth_dev->data->rx_queues[queue_idx];
	struct otx2_eth_dev *dev = otx2_eth_pmd_priv(eth_dev);
	uint32_t head, tail;

	nix_rx_head_tail_get(dev, &head, &tail, queue_idx);
	return (tail - head) % rxq->qlen;
}

static inline int
nix_offset_has_packet(uint32_t head, uint32_t tail, uint16_t offset)
{
	/* Check given offset(queue index) has packet filled by HW */
	if (tail > head && offset <= tail && offset >= head)
		return 1;
	/* Wrap around case */
	if (head > tail && (offset >= head || offset <= tail))
		return 1;

	return 0;
}

int
otx2_nix_rx_descriptor_done(void *rx_queue, uint16_t offset)
{
	struct otx2_eth_rxq *rxq = rx_queue;
	uint32_t head, tail;

	nix_rx_head_tail_get(otx2_eth_pmd_priv(rxq->eth_dev),
			     &head, &tail, rxq->rq);

	return nix_offset_has_packet(head, tail, offset);
}

int
otx2_nix_rx_descriptor_status(void *rx_queue, uint16_t offset)
{
	struct otx2_eth_rxq *rxq = rx_queue;
	uint32_t head, tail;

	if (rxq->qlen >= offset)
		return -EINVAL;

	nix_rx_head_tail_get(otx2_eth_pmd_priv(rxq->eth_dev),
			     &head, &tail, rxq->rq);

	if (nix_offset_has_packet(head, tail, offset))
		return RTE_ETH_RX_DESC_DONE;
	else
		return RTE_ETH_RX_DESC_AVAIL;
}

/* It is a NOP for octeontx2 as HW frees the buffer on xmit */
int
otx2_nix_tx_done_cleanup(void *txq, uint32_t free_cnt)
{
	RTE_SET_USED(txq);
	RTE_SET_USED(free_cnt);

	return 0;
}

int
otx2_nix_fw_version_get(struct rte_eth_dev *eth_dev, char *fw_version,
			size_t fw_size)
{
	struct otx2_eth_dev *dev = otx2_eth_pmd_priv(eth_dev);
	int rc = (int)fw_size;

	if (fw_size > sizeof(dev->mkex_pfl_name))
		rc = sizeof(dev->mkex_pfl_name);

	rc = strlcpy(fw_version, (char *)dev->mkex_pfl_name, rc);

	rc += 1; /* Add the size of '\0' */
	if (fw_size < (uint32_t)rc)
		return rc;

	return 0;
}

int
otx2_nix_pool_ops_supported(struct rte_eth_dev *eth_dev, const char *pool)
{
	RTE_SET_USED(eth_dev);

	if (!strcmp(pool, rte_mbuf_platform_mempool_ops()))
		return 0;

	return -ENOTSUP;
}

int
otx2_nix_dev_filter_ctrl(struct rte_eth_dev *eth_dev,
			 enum rte_filter_type filter_type,
			 enum rte_filter_op filter_op, void *arg)
{
	RTE_SET_USED(eth_dev);

	if (filter_type != RTE_ETH_FILTER_GENERIC) {
		otx2_err("Unsupported filter type %d", filter_type);
		return -ENOTSUP;
	}

	if (filter_op == RTE_ETH_FILTER_GET) {
		*(const void **)arg = &otx2_flow_ops;
		return 0;
	}

	otx2_err("Invalid filter_op %d", filter_op);
	return -EINVAL;
}

static struct cgx_fw_data *
nix_get_fwdata(struct otx2_eth_dev *dev)
{
	struct otx2_mbox *mbox = dev->mbox;
	struct cgx_fw_data *rsp = NULL;

	otx2_mbox_alloc_msg_cgx_get_aux_link_info(mbox);

	otx2_mbox_process_msg(mbox, (void *)&rsp);

	return rsp;
}

int
otx2_nix_get_module_info(struct rte_eth_dev *eth_dev,
			 struct rte_eth_dev_module_info *modinfo)
{
	struct otx2_eth_dev *dev = otx2_eth_pmd_priv(eth_dev);
	struct cgx_fw_data *rsp;

	rsp = nix_get_fwdata(dev);
	if (rsp == NULL)
		return -EIO;

	modinfo->type = rsp->fwdata.sfp_eeprom.sff_id;
	modinfo->eeprom_len = SFP_EEPROM_SIZE;

	return 0;
}

int
otx2_nix_get_module_eeprom(struct rte_eth_dev *eth_dev,
			   struct rte_dev_eeprom_info *info)
{
	struct otx2_eth_dev *dev = otx2_eth_pmd_priv(eth_dev);
	struct cgx_fw_data *rsp;

	if (!info->data || !info->length ||
	    (info->offset + info->length > SFP_EEPROM_SIZE))
		return -EINVAL;

	rsp = nix_get_fwdata(dev);
	if (rsp == NULL)
		return -EIO;

	otx2_mbox_memcpy(info->data, rsp->fwdata.sfp_eeprom.buf + info->offset,
			 info->length);

	return 0;
}

void
otx2_nix_info_get(struct rte_eth_dev *eth_dev, struct rte_eth_dev_info *devinfo)
{
	struct rte_pci_device *pci_dev = RTE_ETH_DEV_TO_PCI(eth_dev);
	struct otx2_eth_dev *dev = otx2_eth_pmd_priv(eth_dev);

	devinfo->min_rx_bufsize = NIX_MIN_FRS;
	devinfo->max_rx_pktlen = NIX_MAX_FRS;
	devinfo->max_rx_queues = RTE_MAX_QUEUES_PER_PORT;
	devinfo->max_tx_queues = RTE_MAX_QUEUES_PER_PORT;
	devinfo->max_mac_addrs = dev->max_mac_entries;
	devinfo->max_vfs = pci_dev->max_vfs;
	devinfo->max_mtu = devinfo->max_rx_pktlen - NIX_L2_OVERHEAD;
	devinfo->min_mtu = devinfo->min_rx_bufsize - NIX_L2_OVERHEAD;

	devinfo->rx_offload_capa = dev->rx_offload_capa;
	devinfo->tx_offload_capa = dev->tx_offload_capa;
	devinfo->rx_queue_offload_capa = 0;
	devinfo->tx_queue_offload_capa = 0;

	devinfo->reta_size = dev->rss_info.rss_size;
	devinfo->hash_key_size = NIX_HASH_KEY_SIZE;
	devinfo->flow_type_rss_offloads = NIX_RSS_OFFLOAD;

	devinfo->default_rxconf = (struct rte_eth_rxconf) {
		.rx_drop_en = 0,
		.offloads = 0,
	};

	devinfo->default_txconf = (struct rte_eth_txconf) {
		.offloads = 0,
	};

	devinfo->rx_desc_lim = (struct rte_eth_desc_lim) {
		.nb_max = UINT16_MAX,
		.nb_min = NIX_RX_MIN_DESC,
		.nb_align = NIX_RX_MIN_DESC_ALIGN,
		.nb_seg_max = NIX_RX_NB_SEG_MAX,
		.nb_mtu_seg_max = NIX_RX_NB_SEG_MAX,
	};
	devinfo->rx_desc_lim.nb_max =
		RTE_ALIGN_MUL_FLOOR(devinfo->rx_desc_lim.nb_max,
				    NIX_RX_MIN_DESC_ALIGN);

	devinfo->tx_desc_lim = (struct rte_eth_desc_lim) {
		.nb_max = UINT16_MAX,
		.nb_min = 1,
		.nb_align = 1,
		.nb_seg_max = NIX_TX_NB_SEG_MAX,
		.nb_mtu_seg_max = NIX_TX_NB_SEG_MAX,
	};

	/* Auto negotiation disabled */
	devinfo->speed_capa = ETH_LINK_SPEED_FIXED;
	devinfo->speed_capa |= ETH_LINK_SPEED_1G | ETH_LINK_SPEED_10G |
				ETH_LINK_SPEED_25G | ETH_LINK_SPEED_40G |
				ETH_LINK_SPEED_50G | ETH_LINK_SPEED_100G;

	devinfo->dev_capa = RTE_ETH_DEV_CAPA_RUNTIME_RX_QUEUE_SETUP |
				RTE_ETH_DEV_CAPA_RUNTIME_TX_QUEUE_SETUP;
}
