/* SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2012-2018 Solarflare Communications Inc.
 * All rights reserved.
 */

/*
 * This is NOT the original source file. Do NOT edit it.
 * To update the board and firmware ids, please edit the copy in
 * the sfregistry repo and then, in that repo,
 * "make id_headers" or "make export" to
 * regenerate and export all types of headers.
 */

#ifndef CI_MGMT_FIRMWARE_IDS_H
#define CI_MGMT_FIRMWARE_IDS_H

/* Reference: SF-103588-PS
 *
 * This header file is the input for v5s/scripts/genfwdef. So if you touch it,
 * ensure that v5/scripts/genfwdef still works.
 */

enum {
  FIRMWARE_TYPE_PHY = 0,
  FIRMWARE_TYPE_PHY_LOADER = 1,
  FIRMWARE_TYPE_BOOTROM = 2,
  FIRMWARE_TYPE_MCFW = 3,
  FIRMWARE_TYPE_MCFW_BACKUP = 4,
  FIRMWARE_TYPE_DISABLED_CALLISTO = 5,
  FIRMWARE_TYPE_FPGA = 6,
  FIRMWARE_TYPE_FPGA_BACKUP = 7,
  FIRMWARE_TYPE_FCFW = 8,
  FIRMWARE_TYPE_FCFW_BACKUP = 9,
  FIRMWARE_TYPE_CPLD = 10,
  FIRMWARE_TYPE_MUMFW = 11,
  FIRMWARE_TYPE_UEFIROM = 12,
  FIRMWARE_TYPE_BUNDLE = 13,
  FIRMWARE_TYPE_CMCFW = 14,
};

enum {
  FIRMWARE_PHY_SUBTYPE_SFX7101B = 0x3,
  FIRMWARE_PHY_SUBTYPE_SFT9001A = 0x8,
  FIRMWARE_PHY_SUBTYPE_QT2025C = 0x9,
  FIRMWARE_PHY_SUBTYPE_SFT9001B = 0xa,
  FIRMWARE_PHY_SUBTYPE_SFL9021 = 0x10,      /* used for loader only */
  FIRMWARE_PHY_SUBTYPE_QT2025_KR = 0x11,    /* QT2025 in KR rather than SFP+ mode */
  FIRMWARE_PHY_SUBTYPE_AEL3020 = 0x12,      /* As seen on the R2 HP blade NIC */
};

enum {
  FIRMWARE_BOOTROM_SUBTYPE_FALCON = 0,
  FIRMWARE_BOOTROM_SUBTYPE_BETHPAGE = 1,
  FIRMWARE_BOOTROM_SUBTYPE_SIENA = 2,
  FIRMWARE_BOOTROM_SUBTYPE_HUNTINGTON = 3,
  FIRMWARE_BOOTROM_SUBTYPE_FARMINGDALE = 4,
  FIRMWARE_BOOTROM_SUBTYPE_GREENPORT = 5,
  FIRMWARE_BOOTROM_SUBTYPE_MEDFORD = 6,
  FIRMWARE_BOOTROM_SUBTYPE_MEDFORD2 = 7,
  FIRMWARE_BOOTROM_SUBTYPE_RIVERHEAD = 8,
};

enum {
  FIRMWARE_MCFW_SUBTYPE_COSIM = 0,
  FIRMWARE_MCFW_SUBTYPE_HALFSPEED = 6,
  FIRMWARE_MCFW_SUBTYPE_FLORENCE = 7,
  FIRMWARE_MCFW_SUBTYPE_ZEBEDEE = 8,
  FIRMWARE_MCFW_SUBTYPE_ERMINTRUDE = 9,
  FIRMWARE_MCFW_SUBTYPE_DYLAN = 10,
  FIRMWARE_MCFW_SUBTYPE_BRIAN = 11,
  FIRMWARE_MCFW_SUBTYPE_DOUGAL = 12,
  FIRMWARE_MCFW_SUBTYPE_MR_RUSTY = 13,
  FIRMWARE_MCFW_SUBTYPE_BUXTON = 14,
  FIRMWARE_MCFW_SUBTYPE_HOPE = 15,
  FIRMWARE_MCFW_SUBTYPE_MR_MCHENRY = 16,
  FIRMWARE_MCFW_SUBTYPE_UNCLE_HAMISH = 17,
  FIRMWARE_MCFW_SUBTYPE_TUTTLE = 18,
  FIRMWARE_MCFW_SUBTYPE_FINLAY = 19,
  FIRMWARE_MCFW_SUBTYPE_KAPTEYN = 20,
  FIRMWARE_MCFW_SUBTYPE_JOHNSON = 21,
  FIRMWARE_MCFW_SUBTYPE_GEHRELS = 22,
  FIRMWARE_MCFW_SUBTYPE_WHIPPLE = 23,
  FIRMWARE_MCFW_SUBTYPE_FORBES = 24,
  FIRMWARE_MCFW_SUBTYPE_LONGMORE = 25,
  FIRMWARE_MCFW_SUBTYPE_HERSCHEL = 26,
  FIRMWARE_MCFW_SUBTYPE_SHOEMAKER = 27,
  FIRMWARE_MCFW_SUBTYPE_IKEYA = 28,
  FIRMWARE_MCFW_SUBTYPE_KOWALSKI = 29,
  FIRMWARE_MCFW_SUBTYPE_NIMRUD = 30,
  FIRMWARE_MCFW_SUBTYPE_SPARTA = 31,
  FIRMWARE_MCFW_SUBTYPE_THEBES = 32,
  FIRMWARE_MCFW_SUBTYPE_ICARUS = 33,
  FIRMWARE_MCFW_SUBTYPE_JERICHO = 34,
  FIRMWARE_MCFW_SUBTYPE_BYBLOS = 35,
  FIRMWARE_MCFW_SUBTYPE_GROAT = 36,
  FIRMWARE_MCFW_SUBTYPE_SHILLING = 37,
  FIRMWARE_MCFW_SUBTYPE_FLORIN = 38,
  FIRMWARE_MCFW_SUBTYPE_THREEPENCE = 39,
  FIRMWARE_MCFW_SUBTYPE_CYCLOPS = 40,
  FIRMWARE_MCFW_SUBTYPE_PENNY = 41,
  FIRMWARE_MCFW_SUBTYPE_BOB = 42,
  FIRMWARE_MCFW_SUBTYPE_HOG = 43,
  FIRMWARE_MCFW_SUBTYPE_SOVEREIGN = 44,
  FIRMWARE_MCFW_SUBTYPE_SOLIDUS = 45,
  FIRMWARE_MCFW_SUBTYPE_SIXPENCE = 46,
  FIRMWARE_MCFW_SUBTYPE_CROWN = 47,
  FIRMWARE_MCFW_SUBTYPE_SOL = 48,
  FIRMWARE_MCFW_SUBTYPE_TANNER = 49,
  FIRMWARE_MCFW_SUBTYPE_BELUGA = 64,
  FIRMWARE_MCFW_SUBTYPE_KALUGA = 65,
};

enum {
  FIRMWARE_DISABLED_CALLISTO_SUBTYPE_ALL = 0
};

enum {
  FIRMWARE_FPGA_SUBTYPE_PTP = 1,                /* PTP peripheral */
  FIRMWARE_FPGA_SUBTYPE_PTP_MR_MCHENRY = 2,     /* PTP peripheral on R7 boards */
  FIRMWARE_FPGA_SUBTYPE_FLORENCE = 3,           /* Modena FPGA */
  FIRMWARE_FPGA_SUBTYPE_UNCLE_HAMISH = 4,       /* Modena FPGA: Unknown silicon */
  FIRMWARE_FPGA_SUBTYPE_UNCLE_HAMISH_A7 = 5,    /* Modena FPGA: A7 silicon */
  FIRMWARE_FPGA_SUBTYPE_UNCLE_HAMISH_A5 = 6,    /* Modena FPGA: A5 silicon */
  FIRMWARE_FPGA_SUBTYPE_SHOEMAKER = 7,          /* Sorrento FPGA: Unknown silicon */
  FIRMWARE_FPGA_SUBTYPE_SHOEMAKER_A5 = 8,       /* Sorrento FPGA: A5 silicon */
  FIRMWARE_FPGA_SUBTYPE_SHOEMAKER_A7 = 9,       /* Sorrento FPGA: A7 silicon */
};

enum {
  FIRMWARE_FCFW_SUBTYPE_MODENA = 1,
  FIRMWARE_FCFW_SUBTYPE_SORRENTO = 2,
};

enum {
  FIRMWARE_CPLD_SUBTYPE_SFA6902 = 1,            /* CPLD on Modena (2-port) */
};

enum {
  FIRMWARE_LICENSE_SUBTYPE_AOE = 1,            /* AOE */
};

enum {
  FIRMWARE_MUMFW_SUBTYPE_MADAM_BLUE = 1,       /* Sorrento MUM firmware */
  FIRMWARE_MUMFW_SUBTYPE_ICARUS = 2,           /* Malaga MUM firmware */
  FIRMWARE_MUMFW_SUBTYPE_JERICHO = 3,          /* Emma MUM firmware */
  FIRMWARE_MUMFW_SUBTYPE_BYBLOS = 4,           /* Pagnell MUM firmware */
  FIRMWARE_MUMFW_SUBTYPE_SHILLING = 5,         /* Bradford R1.x MUM firmware */
  FIRMWARE_MUMFW_SUBTYPE_FLORIN = 6,           /* Bingley MUM firmware */
  FIRMWARE_MUMFW_SUBTYPE_THREEPENCE = 7,       /* Baildon MUM firmware */
  FIRMWARE_MUMFW_SUBTYPE_CYCLOPS = 8,          /* Talbot MUM firmware */
  FIRMWARE_MUMFW_SUBTYPE_PENNY = 9,            /* Batley MUM firmware */
  FIRMWARE_MUMFW_SUBTYPE_BOB = 10,             /* Bradford R2.x MUM firmware */
  FIRMWARE_MUMFW_SUBTYPE_HOG = 11,             /* Roxburgh MUM firmware */
  FIRMWARE_MUMFW_SUBTYPE_SOVEREIGN = 12,       /* Stirling MUM firmware */
  FIRMWARE_MUMFW_SUBTYPE_SOLIDUS = 13,         /* Roxburgh R2 MUM firmware */
  FIRMWARE_MUMFW_SUBTYPE_SIXPENCE = 14,        /* Melrose MUM firmware for Dell cards */
  FIRMWARE_MUMFW_SUBTYPE_CROWN = 15,           /* Coldstream MUM firmware */
  FIRMWARE_MUMFW_SUBTYPE_SOL = 16,             /* Roxburgh R2 MUM firmware for Dell cards with signed-bundle-update */
  FIRMWARE_MUMFW_SUBTYPE_KALUGA = 17,          /* York MUM firmware */
  FIRMWARE_MUMFW_SUBTYPE_STERLET = 18,         /* Bourn MUM firmware */
  FIRMWARE_MUMFW_SUBTYPE_TANNER = 19,          /* Melrose MUM firmware for channel cards */

};


#define FIRMWARE_UEFIROM_SUBTYPE_ALL FIRMWARE_UEFIROM_SUBTYPE_EF10
enum {
  FIRMWARE_UEFIROM_SUBTYPE_EF10 = 0,
};

enum {
  FIRMWARE_BUNDLE_SUBTYPE_DELL_X2522_25G = 1,  /* X2522-25G for Dell with bundle update support */
  FIRMWARE_BUNDLE_SUBTYPE_X2552 = 2,           /* X2552 OCP NIC - firmware bundle */
  FIRMWARE_BUNDLE_SUBTYPE_DELL_X2562 = 3,      /* X2562 OCP NIC for Dell - firmware bundle */
  FIRMWARE_BUNDLE_SUBTYPE_X2562 = 4,           /* X2562 OCP NIC - firmware bundle */
};

enum {
  FIRMWARE_CMCFW_SUBTYPE_BELUGA = 1,           /* Riverhead VCU1525 CMC firmware */
  FIRMWARE_CMCFW_SUBTYPE_KALUGA = 2,           /* York (X3x42) board CMC firmware */
};

#endif
