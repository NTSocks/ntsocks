..  SPDX-License-Identifier: BSD-3-Clause
    Copyright 2019 The DPDK contributors

.. include:: <isonum.txt>

DPDK Release 19.08
==================

.. **Read this first.**

   The text in the sections below explains how to update the release notes.

   Use proper spelling, capitalization and punctuation in all sections.

   Variable and config names should be quoted as fixed width text:
   ``LIKE_THIS``.

   Build the docs and view the output file to ensure the changes are correct::

      make doc-guides-html

      xdg-open build/doc/html/guides/rel_notes/release_19_08.html


New Features
------------

.. This section should contain new features added in this release.
   Sample format:

   * **Add a title in the past tense with a full stop.**

     Add a short 1-2 sentence description in the past tense.
     The description should be enough to allow someone scanning
     the release notes to understand the new feature.

     If the feature adds a lot of sub-features you can use a bullet list
     like this:

     * Added feature foo to do something.
     * Enhanced feature bar to do something else.

     Refer to the previous release notes for examples.

     Suggested order in release notes items:
     * Core libs (EAL, mempool, ring, mbuf, buses)
     * Device abstraction libs and PMDs
       - ethdev (lib, PMDs)
       - cryptodev (lib, PMDs)
       - eventdev (lib, PMDs)
       - etc
     * Other libs
     * Apps, Examples, Tools (if significant)

     This section is a comment. Do not overwrite or remove it.
     Also, make sure to start the actual text at the margin.
     =========================================================

* **Added MCS lock.**

  MCS lock provides scalability by spinning on a CPU/thread local variable
  which avoids expensive cache bouncings.
  It provides fairness by maintaining a list of acquirers and passing
  the lock to each CPU/thread in the order they acquired the lock.

* **Updated the EAL Pseudo-random Number Generator.**

  The lrand48()-based rte_rand() function is replaced with a
  DPDK-native combined Linear Feedback Shift Register (LFSR)
  pseudo-random number generator (PRNG).

  This new PRNG implementation is multi-thread safe, provides
  higher-quality pseudo-random numbers (including full 64 bit
  support) and improved performance.

  In addition, <rte_random.h> is extended with a new function
  rte_rand_max() which supplies unbiased, bounded pseudo-random
  numbers.

* **Updated the bnxt PMD.**

  Updated the bnxt PMD. The major enhancements include:

  * Performance optimizations in non-vector Tx path
  * Added support for SSE vector mode
  * Updated HWRM API to version 1.10.0.74

* **Added support for Broadcom NetXtreme-E BCM57500 Ethernet controllers.**

  Added support to the bnxt PMD for the BCM57500 (a.k.a. "Thor") family
  of Ethernet controllers. These controllers support link speeds up to
  200Gbps, 50G PAM-4, and PCIe 4.0.

* **Added hinic PMD.**

  Added the new ``hinic`` net driver for Huawei Intelligent PCIE Network
  Adapters based on the Huawei Ethernet Controller Hi1822.
  See the :doc:`../nics/hinic` guide for more details on this new driver.

* **Updated the ice driver.**

  Updated ice driver with new features and improvements, including:

  * Enabled Tx outer/inner L3/L4 checksum offload.
  * Enabled generic filter framework and supported switch filter.
  * Supported UDP tunnel port add.

* **Updated Mellanox mlx5 driver.**

  Updated Mellanox mlx5 driver with new features and improvements, including:

  * Updated the packet header modification feature. Added support of TCP header
    sequence number and acknowledgment number modification.
  * Added support for match on ICMP/ICMP6 code and type.

* **Updated Solarflare network PMD.**

  Updated the Solarflare ``sfc_efx`` driver with changes including:

  * Added support for Rx interrupts.

* **Added memif PMD.**

  Added the new Shared Memory Packet Interface (``memif``) PMD.
  See the :doc:`../nics/memif` guide for more details on this new driver.

* **Updated the AF_XDP PMD.**

  Updated the AF_XDP PMD. The new features include:

  * Enabled zero copy through mbuf's external memory mechanism to achieve
    high performance
  * Added multi-queue support to allow one af_xdp vdev with multiple netdev
    queues
  * Enabled need_wakeup feature which can provide efficient support for case
    that application and driver executing on the same core.

* **Enabled infinite Rx in the PCAP PMD.**

  Added an infinite Rx feature which allows packets in the Rx PCAP of a PCAP
  device to be received repeatedly at a high rate. This can be useful for quick
  performance testing of DPDK apps.

* **Added a FPGA_LTE_FEC bbdev PMD.**

  Added the new ``fpga_lte_fec`` bbdev driver for the Intel® FPGA PAC
  (Programmable  Acceleration Card) N3000.  See the
  :doc:`../bbdevs/fpga_lte_fec` BBDEV guide for more details on this new driver.

* **Updated TURBO_SW bbdev PMD.**

  Updated the ``turbo_sw`` bbdev driver with changes including:

  * Added option to build the driver with or without dependency of external
    SDK libraries.
  * Added support for 5GNR encode/decode operations.

* **Updated the QuickAssist Technology (QAT) symmetric crypto PMD.**

  Added support for digest-encrypted cases where digest is appended
  to the data.

* **Added Intel QuickData Technology PMD**

  The PMD for Intel\ |reg|  QuickData Technology, part of
  Intel\ |reg|  I/O Acceleration Technology `(Intel I/OAT)
  <https://www.intel.com/content/www/us/en/wireless-network/accel-technology.html>`_,
  allows data copies to be done by hardware instead
  of via software, reducing cycles spent copying large blocks of data in
  applications.

* **Added Marvell OCTEON TX2 drivers.**

  Added the new ``ethdev``, ``eventdev``, ``mempool``, ``eventdev Rx adapter``,
  ``eventdev Tx adapter``, ``eventdev Timer adapter`` and ``rawdev DMA``
  drivers for various HW coprocessors available in ``OCTEON TX2`` SoC.

  See :doc:`../platform/octeontx2` and driver informations:

  * :doc:`../nics/octeontx2`
  * :doc:`../mempool/octeontx2`
  * :doc:`../eventdevs/octeontx2`
  * :doc:`../rawdevs/octeontx2_dma`

* **Introduced NTB PMD.**

  Added a PMD for Intel NTB (Non-transparent Bridge). This PMD implemented
  handshake between two separate hosts and can share local memory for peer
  host to directly access.

* **Updated IPSec library Header Reconstruction.**

  Updated the IPSec library with ECN and DSCP field header reconstruction
  feature followed by RFC4301. The IPSec-secgw sample application is also
  updated to support this feature by default.

* **Updated telemetry library for global metrics support.**

  Updated ``librte_telemetry`` to fetch the global metrics from the
  ``librte_metrics`` library.

* **Added new telemetry mode for l3fwd-power application.**

  Added telemetry mode to l3fwd-power application to report
  application level busyness, empty and full polls of rte_eth_rx_burst().

* **Updated the pdump application.**

  Add support for pdump to exit with primary process.

* **Updated test-compress-perf tool application.**

  Added multiple cores feature to compression perf tool application.


Removed Items
-------------

.. This section should contain removed items in this release. Sample format:

   * Add a short 1-2 sentence description of the removed item
     in the past tense.

   This section is a comment. Do not overwrite or remove it.
   Also, make sure to start the actual text at the margin.
   =========================================================

* Removed KNI ethtool, CONFIG_RTE_KNI_KMOD_ETHTOOL, support.

* build: armv8 crypto extension is disabled.


API Changes
-----------

.. This section should contain API changes. Sample format:

   * sample: Add a short 1-2 sentence description of the API change
     which was announced in the previous releases and made in this release.
     Start with a scope label like "ethdev:".
     Use fixed width quotes for ``function_names`` or ``struct_names``.
     Use the past tense.

   This section is a comment. Do not overwrite or remove it.
   Also, make sure to start the actual text at the margin.
   =========================================================

* The ``rte_mem_config`` structure has been made private. The new accessor
  ``rte_mcfg_*`` functions were introduced to provide replacement for direct
  access to the shared mem config.

* The network structures, definitions and functions have
  been prefixed by ``rte_`` to resolve conflicts with libc headers.

* malloc: The function ``rte_malloc_set_limit`` was never implemented
  is deprecated and will be removed in a future release.

* cryptodev: the ``uint8_t *data`` member of ``key`` structure in the xforms
  structure (``rte_crypto_cipher_xform``, ``rte_crypto_auth_xform``, and
  ``rte_crypto_aead_xform``) have been changed to ``const uint8_t *data``.

* eventdev: No longer marked as experimental.

  The eventdev functions are no longer marked as experimental, and have
  become part of the normal DPDK API and ABI. Any future ABI changes will be
  announced at least one release before the ABI change is made. There are no
  ABI breaking changes planned.

* ip_frag: IP fragmentation library converts input mbuf into fragments
  using input MTU size via ``rte_ipv4_fragment_packet`` interface.
  Once fragmentation is done, each ``mbuf->ol_flags`` are set to enable IP
  checksum H/W offload irrespective of the platform capability.
  Cleared IP checksum H/W offload flag from the library. The application must
  set this flag if it is supported by the platform and application wishes to
  use it.

* ip_frag: IP reassembly library converts the list of fragments into a
  reassembled packet via ``rte_ipv4_frag_reassemble_packet`` interface.
  Once reassembly is done, ``mbuf->ol_flags`` are set to enable IP checksum H/W
  offload irrespective of the platform capability. Cleared IP checksum H/W
  offload flag from the library. The application must set this flag if it is
  supported by the platform and application wishes to use it.


ABI Changes
-----------

.. This section should contain ABI changes. Sample format:

   * sample: Add a short 1-2 sentence description of the ABI change
     which was announced in the previous releases and made in this release.
     Start with a scope label like "ethdev:".
     Use fixed width quotes for ``function_names`` or ``struct_names``.
     Use the past tense.

   This section is a comment. Do not overwrite or remove it.
   Also, make sure to start the actual text at the margin.
   =========================================================

* eventdev: Event based Rx adapter callback

  The mbuf pointer array in the event eth Rx adapter callback
  has been replaced with an event array. Using
  an event array allows the application to change attributes
  of the events enqueued by the SW adapter.

  The callback can drop packets and populate
  a callback argument with the number of dropped packets.
  Add a Rx adapter stats field to keep track of the total
  number of dropped packets.

* cryptodev: New member in ``rte_cryptodev_config`` to allow applications to
  disable features supported by the crypto device. Only the following features
  would be allowed to be disabled this way,

  - ``RTE_CRYPTODEV_FF_SYMMETRIC_CRYPTO``
  - ``RTE_CRYPTODEV_FF_ASYMMETRIC_CRYPTO``
  - ``RTE_CRYPTODEV_FF_SECURITY``

  Disabling unused features would facilitate efficient usage of HW/SW offload.

* bbdev: New operations and parameters added to support new 5GNR operations.
  The bbdev ABI is still kept experimental.


Shared Library Versions
-----------------------

.. Update any library version updated in this release
   and prepend with a ``+`` sign, like this:

     libfoo.so.1
   + libupdated.so.2
     libbar.so.1

   This section is a comment. Do not overwrite or remove it.
   =========================================================

The libraries prepended with a plus sign were incremented in this version.

.. code-block:: diff

     librte_acl.so.2
     librte_bbdev.so.1
     librte_bitratestats.so.2
     librte_bpf.so.1
     librte_bus_dpaa.so.2
     librte_bus_fslmc.so.2
     librte_bus_ifpga.so.2
     librte_bus_pci.so.2
     librte_bus_vdev.so.2
     librte_bus_vmbus.so.2
     librte_cfgfile.so.2
     librte_cmdline.so.2
     librte_compressdev.so.1
   + librte_cryptodev.so.8
     librte_distributor.so.1
   + librte_eal.so.11
     librte_efd.so.1
     librte_ethdev.so.12
   + librte_eventdev.so.7
     librte_flow_classify.so.1
     librte_gro.so.1
     librte_gso.so.1
     librte_hash.so.2
     librte_ip_frag.so.1
     librte_ipsec.so.1
     librte_jobstats.so.1
     librte_kni.so.2
     librte_kvargs.so.1
     librte_latencystats.so.1
     librte_lpm.so.2
     librte_mbuf.so.5
     librte_member.so.1
     librte_mempool.so.5
     librte_meter.so.3
     librte_metrics.so.1
     librte_net.so.1
     librte_pci.so.1
     librte_pdump.so.3
     librte_pipeline.so.3
     librte_pmd_bnxt.so.2
     librte_pmd_bond.so.2
     librte_pmd_i40e.so.2
     librte_pmd_ixgbe.so.2
     librte_pmd_dpaa2_qdma.so.1
     librte_pmd_ring.so.2
     librte_pmd_softnic.so.1
     librte_pmd_vhost.so.2
     librte_port.so.3
     librte_power.so.1
     librte_rawdev.so.1
     librte_rcu.so.1
     librte_reorder.so.1
     librte_ring.so.2
     librte_sched.so.2
     librte_security.so.2
     librte_stack.so.1
     librte_table.so.3
     librte_timer.so.1
     librte_vhost.so.4


Known Issues
------------

.. This section should contain new known issues in this release. Sample format:

   * **Add title in present tense with full stop.**

     Add a short 1-2 sentence description of the known issue
     in the present tense. Add information on any known workarounds.

   This section is a comment. Do not overwrite or remove it.
   Also, make sure to start the actual text at the margin.
   =========================================================


Tested Platforms
----------------

.. This section should contain a list of platforms that were tested
   with this release.

   The format is:

   * <vendor> platform with <vendor> <type of devices> combinations

     * List of CPU
     * List of OS
     * List of devices
     * Other relevant details...

   This section is a comment. Do not overwrite or remove it.
   Also, make sure to start the actual text at the margin.
   =========================================================

