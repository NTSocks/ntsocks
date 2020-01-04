/*******************************************************************************

Copyright (c) 2013 - 2015, Intel Corporation
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 1. Redistributions of source code must retain the above copyright notice,
    this list of conditions and the following disclaimer.

 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.

 3. Neither the name of the Intel Corporation nor the names of its
    contributors may be used to endorse or promote products derived from
    this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.

***************************************************************************/

#ifndef _IAVF_REGISTER_H_
#define _IAVF_REGISTER_H_


#define IAVFMSIX_PBA1(_i)          (0x00002000 + ((_i) * 4)) /* _i=0...19 */ /* Reset: VFLR */
#define IAVFMSIX_PBA1_MAX_INDEX    19
#define IAVFMSIX_PBA1_PENBIT_SHIFT 0
#define IAVFMSIX_PBA1_PENBIT_MASK  IAVF_MASK(0xFFFFFFFF, IAVFMSIX_PBA1_PENBIT_SHIFT)
#define IAVFMSIX_TADD1(_i)              (0x00002100 + ((_i) * 16)) /* _i=0...639 */ /* Reset: VFLR */
#define IAVFMSIX_TADD1_MAX_INDEX        639
#define IAVFMSIX_TADD1_MSIXTADD10_SHIFT 0
#define IAVFMSIX_TADD1_MSIXTADD10_MASK  IAVF_MASK(0x3, IAVFMSIX_TADD1_MSIXTADD10_SHIFT)
#define IAVFMSIX_TADD1_MSIXTADD_SHIFT   2
#define IAVFMSIX_TADD1_MSIXTADD_MASK    IAVF_MASK(0x3FFFFFFF, IAVFMSIX_TADD1_MSIXTADD_SHIFT)
#define IAVFMSIX_TMSG1(_i)            (0x00002108 + ((_i) * 16)) /* _i=0...639 */ /* Reset: VFLR */
#define IAVFMSIX_TMSG1_MAX_INDEX      639
#define IAVFMSIX_TMSG1_MSIXTMSG_SHIFT 0
#define IAVFMSIX_TMSG1_MSIXTMSG_MASK  IAVF_MASK(0xFFFFFFFF, IAVFMSIX_TMSG1_MSIXTMSG_SHIFT)
#define IAVFMSIX_TUADD1(_i)             (0x00002104 + ((_i) * 16)) /* _i=0...639 */ /* Reset: VFLR */
#define IAVFMSIX_TUADD1_MAX_INDEX       639
#define IAVFMSIX_TUADD1_MSIXTUADD_SHIFT 0
#define IAVFMSIX_TUADD1_MSIXTUADD_MASK  IAVF_MASK(0xFFFFFFFF, IAVFMSIX_TUADD1_MSIXTUADD_SHIFT)
#define IAVFMSIX_TVCTRL1(_i)        (0x0000210C + ((_i) * 16)) /* _i=0...639 */ /* Reset: VFLR */
#define IAVFMSIX_TVCTRL1_MAX_INDEX  639
#define IAVFMSIX_TVCTRL1_MASK_SHIFT 0
#define IAVFMSIX_TVCTRL1_MASK_MASK  IAVF_MASK(0x1, IAVFMSIX_TVCTRL1_MASK_SHIFT)
#define IAVF_ARQBAH1              0x00006000 /* Reset: EMPR */
#define IAVF_ARQBAH1_ARQBAH_SHIFT 0
#define IAVF_ARQBAH1_ARQBAH_MASK  IAVF_MASK(0xFFFFFFFF, IAVF_ARQBAH1_ARQBAH_SHIFT)
#define IAVF_ARQBAL1              0x00006C00 /* Reset: EMPR */
#define IAVF_ARQBAL1_ARQBAL_SHIFT 0
#define IAVF_ARQBAL1_ARQBAL_MASK  IAVF_MASK(0xFFFFFFFF, IAVF_ARQBAL1_ARQBAL_SHIFT)
#define IAVF_ARQH1            0x00007400 /* Reset: EMPR */
#define IAVF_ARQH1_ARQH_SHIFT 0
#define IAVF_ARQH1_ARQH_MASK  IAVF_MASK(0x3FF, IAVF_ARQH1_ARQH_SHIFT)
#define IAVF_ARQLEN1                 0x00008000 /* Reset: EMPR */
#define IAVF_ARQLEN1_ARQLEN_SHIFT    0
#define IAVF_ARQLEN1_ARQLEN_MASK     IAVF_MASK(0x3FF, IAVF_ARQLEN1_ARQLEN_SHIFT)
#define IAVF_ARQLEN1_ARQVFE_SHIFT    28
#define IAVF_ARQLEN1_ARQVFE_MASK     IAVF_MASK(0x1, IAVF_ARQLEN1_ARQVFE_SHIFT)
#define IAVF_ARQLEN1_ARQOVFL_SHIFT   29
#define IAVF_ARQLEN1_ARQOVFL_MASK    IAVF_MASK(0x1, IAVF_ARQLEN1_ARQOVFL_SHIFT)
#define IAVF_ARQLEN1_ARQCRIT_SHIFT   30
#define IAVF_ARQLEN1_ARQCRIT_MASK    IAVF_MASK(0x1, IAVF_ARQLEN1_ARQCRIT_SHIFT)
#define IAVF_ARQLEN1_ARQENABLE_SHIFT 31
#define IAVF_ARQLEN1_ARQENABLE_MASK  IAVF_MASK(0x1U, IAVF_ARQLEN1_ARQENABLE_SHIFT)
#define IAVF_ARQT1            0x00007000 /* Reset: EMPR */
#define IAVF_ARQT1_ARQT_SHIFT 0
#define IAVF_ARQT1_ARQT_MASK  IAVF_MASK(0x3FF, IAVF_ARQT1_ARQT_SHIFT)
#define IAVF_ATQBAH1              0x00007800 /* Reset: EMPR */
#define IAVF_ATQBAH1_ATQBAH_SHIFT 0
#define IAVF_ATQBAH1_ATQBAH_MASK  IAVF_MASK(0xFFFFFFFF, IAVF_ATQBAH1_ATQBAH_SHIFT)
#define IAVF_ATQBAL1              0x00007C00 /* Reset: EMPR */
#define IAVF_ATQBAL1_ATQBAL_SHIFT 0
#define IAVF_ATQBAL1_ATQBAL_MASK  IAVF_MASK(0xFFFFFFFF, IAVF_ATQBAL1_ATQBAL_SHIFT)
#define IAVF_ATQH1            0x00006400 /* Reset: EMPR */
#define IAVF_ATQH1_ATQH_SHIFT 0
#define IAVF_ATQH1_ATQH_MASK  IAVF_MASK(0x3FF, IAVF_ATQH1_ATQH_SHIFT)
#define IAVF_ATQLEN1                 0x00006800 /* Reset: EMPR */
#define IAVF_ATQLEN1_ATQLEN_SHIFT    0
#define IAVF_ATQLEN1_ATQLEN_MASK     IAVF_MASK(0x3FF, IAVF_ATQLEN1_ATQLEN_SHIFT)
#define IAVF_ATQLEN1_ATQVFE_SHIFT    28
#define IAVF_ATQLEN1_ATQVFE_MASK     IAVF_MASK(0x1, IAVF_ATQLEN1_ATQVFE_SHIFT)
#define IAVF_ATQLEN1_ATQOVFL_SHIFT   29
#define IAVF_ATQLEN1_ATQOVFL_MASK    IAVF_MASK(0x1, IAVF_ATQLEN1_ATQOVFL_SHIFT)
#define IAVF_ATQLEN1_ATQCRIT_SHIFT   30
#define IAVF_ATQLEN1_ATQCRIT_MASK    IAVF_MASK(0x1, IAVF_ATQLEN1_ATQCRIT_SHIFT)
#define IAVF_ATQLEN1_ATQENABLE_SHIFT 31
#define IAVF_ATQLEN1_ATQENABLE_MASK  IAVF_MASK(0x1U, IAVF_ATQLEN1_ATQENABLE_SHIFT)
#define IAVF_ATQT1            0x00008400 /* Reset: EMPR */
#define IAVF_ATQT1_ATQT_SHIFT 0
#define IAVF_ATQT1_ATQT_MASK  IAVF_MASK(0x3FF, IAVF_ATQT1_ATQT_SHIFT)
#define IAVFGEN_RSTAT                 0x00008800 /* Reset: VFR */
#define IAVFGEN_RSTAT_VFR_STATE_SHIFT 0
#define IAVFGEN_RSTAT_VFR_STATE_MASK  IAVF_MASK(0x3, IAVFGEN_RSTAT_VFR_STATE_SHIFT)
#define IAVFINT_DYN_CTL01                       0x00005C00 /* Reset: VFR */
#define IAVFINT_DYN_CTL01_INTENA_SHIFT          0
#define IAVFINT_DYN_CTL01_INTENA_MASK           IAVF_MASK(0x1, IAVFINT_DYN_CTL01_INTENA_SHIFT)
#define IAVFINT_DYN_CTL01_CLEARPBA_SHIFT        1
#define IAVFINT_DYN_CTL01_CLEARPBA_MASK         IAVF_MASK(0x1, IAVFINT_DYN_CTL01_CLEARPBA_SHIFT)
#define IAVFINT_DYN_CTL01_SWINT_TRIG_SHIFT      2
#define IAVFINT_DYN_CTL01_SWINT_TRIG_MASK       IAVF_MASK(0x1, IAVFINT_DYN_CTL01_SWINT_TRIG_SHIFT)
#define IAVFINT_DYN_CTL01_ITR_INDX_SHIFT        3
#define IAVFINT_DYN_CTL01_ITR_INDX_MASK         IAVF_MASK(0x3, IAVFINT_DYN_CTL01_ITR_INDX_SHIFT)
#define IAVFINT_DYN_CTL01_INTERVAL_SHIFT        5
#define IAVFINT_DYN_CTL01_INTERVAL_MASK         IAVF_MASK(0xFFF, IAVFINT_DYN_CTL01_INTERVAL_SHIFT)
#define IAVFINT_DYN_CTL01_SW_ITR_INDX_ENA_SHIFT 24
#define IAVFINT_DYN_CTL01_SW_ITR_INDX_ENA_MASK  IAVF_MASK(0x1, IAVFINT_DYN_CTL01_SW_ITR_INDX_ENA_SHIFT)
#define IAVFINT_DYN_CTL01_SW_ITR_INDX_SHIFT     25
#define IAVFINT_DYN_CTL01_SW_ITR_INDX_MASK      IAVF_MASK(0x3, IAVFINT_DYN_CTL01_SW_ITR_INDX_SHIFT)
#define IAVFINT_DYN_CTL01_INTENA_MSK_SHIFT      31
#define IAVFINT_DYN_CTL01_INTENA_MSK_MASK       IAVF_MASK(0x1, IAVFINT_DYN_CTL01_INTENA_MSK_SHIFT)
#define IAVFINT_DYN_CTLN1(_INTVF)               (0x00003800 + ((_INTVF) * 4)) /* _i=0...15 */ /* Reset: VFR */
#define IAVFINT_DYN_CTLN1_MAX_INDEX             15
#define IAVFINT_DYN_CTLN1_INTENA_SHIFT          0
#define IAVFINT_DYN_CTLN1_INTENA_MASK           IAVF_MASK(0x1, IAVFINT_DYN_CTLN1_INTENA_SHIFT)
#define IAVFINT_DYN_CTLN1_CLEARPBA_SHIFT        1
#define IAVFINT_DYN_CTLN1_CLEARPBA_MASK         IAVF_MASK(0x1, IAVFINT_DYN_CTLN1_CLEARPBA_SHIFT)
#define IAVFINT_DYN_CTLN1_SWINT_TRIG_SHIFT      2
#define IAVFINT_DYN_CTLN1_SWINT_TRIG_MASK       IAVF_MASK(0x1, IAVFINT_DYN_CTLN1_SWINT_TRIG_SHIFT)
#define IAVFINT_DYN_CTLN1_ITR_INDX_SHIFT        3
#define IAVFINT_DYN_CTLN1_ITR_INDX_MASK         IAVF_MASK(0x3, IAVFINT_DYN_CTLN1_ITR_INDX_SHIFT)
#define IAVFINT_DYN_CTLN1_INTERVAL_SHIFT        5
#define IAVFINT_DYN_CTLN1_INTERVAL_MASK         IAVF_MASK(0xFFF, IAVFINT_DYN_CTLN1_INTERVAL_SHIFT)
#define IAVFINT_DYN_CTLN1_SW_ITR_INDX_ENA_SHIFT 24
#define IAVFINT_DYN_CTLN1_SW_ITR_INDX_ENA_MASK  IAVF_MASK(0x1, IAVFINT_DYN_CTLN1_SW_ITR_INDX_ENA_SHIFT)
#define IAVFINT_DYN_CTLN1_SW_ITR_INDX_SHIFT     25
#define IAVFINT_DYN_CTLN1_SW_ITR_INDX_MASK      IAVF_MASK(0x3, IAVFINT_DYN_CTLN1_SW_ITR_INDX_SHIFT)
#define IAVFINT_DYN_CTLN1_INTENA_MSK_SHIFT      31
#define IAVFINT_DYN_CTLN1_INTENA_MSK_MASK       IAVF_MASK(0x1, IAVFINT_DYN_CTLN1_INTENA_MSK_SHIFT)
#define IAVFINT_ICR0_ENA1                        0x00005000 /* Reset: CORER */
#define IAVFINT_ICR0_ENA1_LINK_STAT_CHANGE_SHIFT 25
#define IAVFINT_ICR0_ENA1_LINK_STAT_CHANGE_MASK  IAVF_MASK(0x1, IAVFINT_ICR0_ENA1_LINK_STAT_CHANGE_SHIFT)
#define IAVFINT_ICR0_ENA1_ADMINQ_SHIFT           30
#define IAVFINT_ICR0_ENA1_ADMINQ_MASK            IAVF_MASK(0x1, IAVFINT_ICR0_ENA1_ADMINQ_SHIFT)
#define IAVFINT_ICR0_ENA1_RSVD_SHIFT             31
#define IAVFINT_ICR0_ENA1_RSVD_MASK              IAVF_MASK(0x1, IAVFINT_ICR0_ENA1_RSVD_SHIFT)
#define IAVFINT_ICR01                        0x00004800 /* Reset: CORER */
#define IAVFINT_ICR01_INTEVENT_SHIFT         0
#define IAVFINT_ICR01_INTEVENT_MASK          IAVF_MASK(0x1, IAVFINT_ICR01_INTEVENT_SHIFT)
#define IAVFINT_ICR01_QUEUE_0_SHIFT          1
#define IAVFINT_ICR01_QUEUE_0_MASK           IAVF_MASK(0x1, IAVFINT_ICR01_QUEUE_0_SHIFT)
#define IAVFINT_ICR01_QUEUE_1_SHIFT          2
#define IAVFINT_ICR01_QUEUE_1_MASK           IAVF_MASK(0x1, IAVFINT_ICR01_QUEUE_1_SHIFT)
#define IAVFINT_ICR01_QUEUE_2_SHIFT          3
#define IAVFINT_ICR01_QUEUE_2_MASK           IAVF_MASK(0x1, IAVFINT_ICR01_QUEUE_2_SHIFT)
#define IAVFINT_ICR01_QUEUE_3_SHIFT          4
#define IAVFINT_ICR01_QUEUE_3_MASK           IAVF_MASK(0x1, IAVFINT_ICR01_QUEUE_3_SHIFT)
#define IAVFINT_ICR01_LINK_STAT_CHANGE_SHIFT 25
#define IAVFINT_ICR01_LINK_STAT_CHANGE_MASK  IAVF_MASK(0x1, IAVFINT_ICR01_LINK_STAT_CHANGE_SHIFT)
#define IAVFINT_ICR01_ADMINQ_SHIFT           30
#define IAVFINT_ICR01_ADMINQ_MASK            IAVF_MASK(0x1, IAVFINT_ICR01_ADMINQ_SHIFT)
#define IAVFINT_ICR01_SWINT_SHIFT            31
#define IAVFINT_ICR01_SWINT_MASK             IAVF_MASK(0x1, IAVFINT_ICR01_SWINT_SHIFT)
#define IAVFINT_ITR01(_i)            (0x00004C00 + ((_i) * 4)) /* _i=0...2 */ /* Reset: VFR */
#define IAVFINT_ITR01_MAX_INDEX      2
#define IAVFINT_ITR01_INTERVAL_SHIFT 0
#define IAVFINT_ITR01_INTERVAL_MASK  IAVF_MASK(0xFFF, IAVFINT_ITR01_INTERVAL_SHIFT)
#define IAVFINT_ITRN1(_i, _INTVF)     (0x00002800 + ((_i) * 64 + (_INTVF) * 4)) /* _i=0...2, _INTVF=0...15 */ /* Reset: VFR */
#define IAVFINT_ITRN1_MAX_INDEX      2
#define IAVFINT_ITRN1_INTERVAL_SHIFT 0
#define IAVFINT_ITRN1_INTERVAL_MASK  IAVF_MASK(0xFFF, IAVFINT_ITRN1_INTERVAL_SHIFT)
#define IAVFINT_STAT_CTL01                      0x00005400 /* Reset: CORER */
#define IAVFINT_STAT_CTL01_OTHER_ITR_INDX_SHIFT 2
#define IAVFINT_STAT_CTL01_OTHER_ITR_INDX_MASK  IAVF_MASK(0x3, IAVFINT_STAT_CTL01_OTHER_ITR_INDX_SHIFT)
#define IAVF_QRX_TAIL1(_Q)        (0x00002000 + ((_Q) * 4)) /* _i=0...15 */ /* Reset: CORER */
#define IAVF_QRX_TAIL1_MAX_INDEX  15
#define IAVF_QRX_TAIL1_TAIL_SHIFT 0
#define IAVF_QRX_TAIL1_TAIL_MASK  IAVF_MASK(0x1FFF, IAVF_QRX_TAIL1_TAIL_SHIFT)
#define IAVF_QTX_TAIL1(_Q)        (0x00000000 + ((_Q) * 4)) /* _i=0...15 */ /* Reset: PFR */
#define IAVF_QTX_TAIL1_MAX_INDEX  15
#define IAVF_QTX_TAIL1_TAIL_SHIFT 0
#define IAVF_QTX_TAIL1_TAIL_MASK  IAVF_MASK(0x1FFF, IAVF_QTX_TAIL1_TAIL_SHIFT)
#define IAVFMSIX_PBA              0x00002000 /* Reset: VFLR */
#define IAVFMSIX_PBA_PENBIT_SHIFT 0
#define IAVFMSIX_PBA_PENBIT_MASK  IAVF_MASK(0xFFFFFFFF, IAVFMSIX_PBA_PENBIT_SHIFT)
#define IAVFMSIX_TADD(_i)              (0x00000000 + ((_i) * 16)) /* _i=0...16 */ /* Reset: VFLR */
#define IAVFMSIX_TADD_MAX_INDEX        16
#define IAVFMSIX_TADD_MSIXTADD10_SHIFT 0
#define IAVFMSIX_TADD_MSIXTADD10_MASK  IAVF_MASK(0x3, IAVFMSIX_TADD_MSIXTADD10_SHIFT)
#define IAVFMSIX_TADD_MSIXTADD_SHIFT   2
#define IAVFMSIX_TADD_MSIXTADD_MASK    IAVF_MASK(0x3FFFFFFF, IAVFMSIX_TADD_MSIXTADD_SHIFT)
#define IAVFMSIX_TMSG(_i)            (0x00000008 + ((_i) * 16)) /* _i=0...16 */ /* Reset: VFLR */
#define IAVFMSIX_TMSG_MAX_INDEX      16
#define IAVFMSIX_TMSG_MSIXTMSG_SHIFT 0
#define IAVFMSIX_TMSG_MSIXTMSG_MASK  IAVF_MASK(0xFFFFFFFF, IAVFMSIX_TMSG_MSIXTMSG_SHIFT)
#define IAVFMSIX_TUADD(_i)             (0x00000004 + ((_i) * 16)) /* _i=0...16 */ /* Reset: VFLR */
#define IAVFMSIX_TUADD_MAX_INDEX       16
#define IAVFMSIX_TUADD_MSIXTUADD_SHIFT 0
#define IAVFMSIX_TUADD_MSIXTUADD_MASK  IAVF_MASK(0xFFFFFFFF, IAVFMSIX_TUADD_MSIXTUADD_SHIFT)
#define IAVFMSIX_TVCTRL(_i)        (0x0000000C + ((_i) * 16)) /* _i=0...16 */ /* Reset: VFLR */
#define IAVFMSIX_TVCTRL_MAX_INDEX  16
#define IAVFMSIX_TVCTRL_MASK_SHIFT 0
#define IAVFMSIX_TVCTRL_MASK_MASK  IAVF_MASK(0x1, IAVFMSIX_TVCTRL_MASK_SHIFT)
#define IAVFCM_PE_ERRDATA                  0x0000DC00 /* Reset: VFR */
#define IAVFCM_PE_ERRDATA_ERROR_CODE_SHIFT 0
#define IAVFCM_PE_ERRDATA_ERROR_CODE_MASK  IAVF_MASK(0xF, IAVFCM_PE_ERRDATA_ERROR_CODE_SHIFT)
#define IAVFCM_PE_ERRDATA_Q_TYPE_SHIFT     4
#define IAVFCM_PE_ERRDATA_Q_TYPE_MASK      IAVF_MASK(0x7, IAVFCM_PE_ERRDATA_Q_TYPE_SHIFT)
#define IAVFCM_PE_ERRDATA_Q_NUM_SHIFT      8
#define IAVFCM_PE_ERRDATA_Q_NUM_MASK       IAVF_MASK(0x3FFFF, IAVFCM_PE_ERRDATA_Q_NUM_SHIFT)
#define IAVFCM_PE_ERRINFO                     0x0000D800 /* Reset: VFR */
#define IAVFCM_PE_ERRINFO_ERROR_VALID_SHIFT   0
#define IAVFCM_PE_ERRINFO_ERROR_VALID_MASK    IAVF_MASK(0x1, IAVFCM_PE_ERRINFO_ERROR_VALID_SHIFT)
#define IAVFCM_PE_ERRINFO_ERROR_INST_SHIFT    4
#define IAVFCM_PE_ERRINFO_ERROR_INST_MASK     IAVF_MASK(0x7, IAVFCM_PE_ERRINFO_ERROR_INST_SHIFT)
#define IAVFCM_PE_ERRINFO_DBL_ERROR_CNT_SHIFT 8
#define IAVFCM_PE_ERRINFO_DBL_ERROR_CNT_MASK  IAVF_MASK(0xFF, IAVFCM_PE_ERRINFO_DBL_ERROR_CNT_SHIFT)
#define IAVFCM_PE_ERRINFO_RLU_ERROR_CNT_SHIFT 16
#define IAVFCM_PE_ERRINFO_RLU_ERROR_CNT_MASK  IAVF_MASK(0xFF, IAVFCM_PE_ERRINFO_RLU_ERROR_CNT_SHIFT)
#define IAVFCM_PE_ERRINFO_RLS_ERROR_CNT_SHIFT 24
#define IAVFCM_PE_ERRINFO_RLS_ERROR_CNT_MASK  IAVF_MASK(0xFF, IAVFCM_PE_ERRINFO_RLS_ERROR_CNT_SHIFT)
#define IAVFQF_HENA(_i)             (0x0000C400 + ((_i) * 4)) /* _i=0...1 */ /* Reset: CORER */
#define IAVFQF_HENA_MAX_INDEX       1
#define IAVFQF_HENA_PTYPE_ENA_SHIFT 0
#define IAVFQF_HENA_PTYPE_ENA_MASK  IAVF_MASK(0xFFFFFFFF, IAVFQF_HENA_PTYPE_ENA_SHIFT)
#define IAVFQF_HKEY(_i)         (0x0000CC00 + ((_i) * 4)) /* _i=0...12 */ /* Reset: CORER */
#define IAVFQF_HKEY_MAX_INDEX   12
#define IAVFQF_HKEY_KEY_0_SHIFT 0
#define IAVFQF_HKEY_KEY_0_MASK  IAVF_MASK(0xFF, IAVFQF_HKEY_KEY_0_SHIFT)
#define IAVFQF_HKEY_KEY_1_SHIFT 8
#define IAVFQF_HKEY_KEY_1_MASK  IAVF_MASK(0xFF, IAVFQF_HKEY_KEY_1_SHIFT)
#define IAVFQF_HKEY_KEY_2_SHIFT 16
#define IAVFQF_HKEY_KEY_2_MASK  IAVF_MASK(0xFF, IAVFQF_HKEY_KEY_2_SHIFT)
#define IAVFQF_HKEY_KEY_3_SHIFT 24
#define IAVFQF_HKEY_KEY_3_MASK  IAVF_MASK(0xFF, IAVFQF_HKEY_KEY_3_SHIFT)
#define IAVFQF_HLUT(_i)        (0x0000D000 + ((_i) * 4)) /* _i=0...15 */ /* Reset: CORER */
#define IAVFQF_HLUT_MAX_INDEX  15
#define IAVFQF_HLUT_LUT0_SHIFT 0
#define IAVFQF_HLUT_LUT0_MASK  IAVF_MASK(0xF, IAVFQF_HLUT_LUT0_SHIFT)
#define IAVFQF_HLUT_LUT1_SHIFT 8
#define IAVFQF_HLUT_LUT1_MASK  IAVF_MASK(0xF, IAVFQF_HLUT_LUT1_SHIFT)
#define IAVFQF_HLUT_LUT2_SHIFT 16
#define IAVFQF_HLUT_LUT2_MASK  IAVF_MASK(0xF, IAVFQF_HLUT_LUT2_SHIFT)
#define IAVFQF_HLUT_LUT3_SHIFT 24
#define IAVFQF_HLUT_LUT3_MASK  IAVF_MASK(0xF, IAVFQF_HLUT_LUT3_SHIFT)
#define IAVFQF_HREGION(_i)                  (0x0000D400 + ((_i) * 4)) /* _i=0...7 */ /* Reset: CORER */
#define IAVFQF_HREGION_MAX_INDEX            7
#define IAVFQF_HREGION_OVERRIDE_ENA_0_SHIFT 0
#define IAVFQF_HREGION_OVERRIDE_ENA_0_MASK  IAVF_MASK(0x1, IAVFQF_HREGION_OVERRIDE_ENA_0_SHIFT)
#define IAVFQF_HREGION_REGION_0_SHIFT       1
#define IAVFQF_HREGION_REGION_0_MASK        IAVF_MASK(0x7, IAVFQF_HREGION_REGION_0_SHIFT)
#define IAVFQF_HREGION_OVERRIDE_ENA_1_SHIFT 4
#define IAVFQF_HREGION_OVERRIDE_ENA_1_MASK  IAVF_MASK(0x1, IAVFQF_HREGION_OVERRIDE_ENA_1_SHIFT)
#define IAVFQF_HREGION_REGION_1_SHIFT       5
#define IAVFQF_HREGION_REGION_1_MASK        IAVF_MASK(0x7, IAVFQF_HREGION_REGION_1_SHIFT)
#define IAVFQF_HREGION_OVERRIDE_ENA_2_SHIFT 8
#define IAVFQF_HREGION_OVERRIDE_ENA_2_MASK  IAVF_MASK(0x1, IAVFQF_HREGION_OVERRIDE_ENA_2_SHIFT)
#define IAVFQF_HREGION_REGION_2_SHIFT       9
#define IAVFQF_HREGION_REGION_2_MASK        IAVF_MASK(0x7, IAVFQF_HREGION_REGION_2_SHIFT)
#define IAVFQF_HREGION_OVERRIDE_ENA_3_SHIFT 12
#define IAVFQF_HREGION_OVERRIDE_ENA_3_MASK  IAVF_MASK(0x1, IAVFQF_HREGION_OVERRIDE_ENA_3_SHIFT)
#define IAVFQF_HREGION_REGION_3_SHIFT       13
#define IAVFQF_HREGION_REGION_3_MASK        IAVF_MASK(0x7, IAVFQF_HREGION_REGION_3_SHIFT)
#define IAVFQF_HREGION_OVERRIDE_ENA_4_SHIFT 16
#define IAVFQF_HREGION_OVERRIDE_ENA_4_MASK  IAVF_MASK(0x1, IAVFQF_HREGION_OVERRIDE_ENA_4_SHIFT)
#define IAVFQF_HREGION_REGION_4_SHIFT       17
#define IAVFQF_HREGION_REGION_4_MASK        IAVF_MASK(0x7, IAVFQF_HREGION_REGION_4_SHIFT)
#define IAVFQF_HREGION_OVERRIDE_ENA_5_SHIFT 20
#define IAVFQF_HREGION_OVERRIDE_ENA_5_MASK  IAVF_MASK(0x1, IAVFQF_HREGION_OVERRIDE_ENA_5_SHIFT)
#define IAVFQF_HREGION_REGION_5_SHIFT       21
#define IAVFQF_HREGION_REGION_5_MASK        IAVF_MASK(0x7, IAVFQF_HREGION_REGION_5_SHIFT)
#define IAVFQF_HREGION_OVERRIDE_ENA_6_SHIFT 24
#define IAVFQF_HREGION_OVERRIDE_ENA_6_MASK  IAVF_MASK(0x1, IAVFQF_HREGION_OVERRIDE_ENA_6_SHIFT)
#define IAVFQF_HREGION_REGION_6_SHIFT       25
#define IAVFQF_HREGION_REGION_6_MASK        IAVF_MASK(0x7, IAVFQF_HREGION_REGION_6_SHIFT)
#define IAVFQF_HREGION_OVERRIDE_ENA_7_SHIFT 28
#define IAVFQF_HREGION_OVERRIDE_ENA_7_MASK  IAVF_MASK(0x1, IAVFQF_HREGION_OVERRIDE_ENA_7_SHIFT)
#define IAVFQF_HREGION_REGION_7_SHIFT       29
#define IAVFQF_HREGION_REGION_7_MASK        IAVF_MASK(0x7, IAVFQF_HREGION_REGION_7_SHIFT)

#define IAVFINT_DYN_CTL01_WB_ON_ITR_SHIFT       30
#define IAVFINT_DYN_CTL01_WB_ON_ITR_MASK        IAVF_MASK(0x1, IAVFINT_DYN_CTL01_WB_ON_ITR_SHIFT)
#define IAVFINT_DYN_CTLN1_WB_ON_ITR_SHIFT       30
#define IAVFINT_DYN_CTLN1_WB_ON_ITR_MASK        IAVF_MASK(0x1, IAVFINT_DYN_CTLN1_WB_ON_ITR_SHIFT)
#define IAVFPE_AEQALLOC1               0x0000A400 /* Reset: VFR */
#define IAVFPE_AEQALLOC1_AECOUNT_SHIFT 0
#define IAVFPE_AEQALLOC1_AECOUNT_MASK  IAVF_MASK(0xFFFFFFFF, IAVFPE_AEQALLOC1_AECOUNT_SHIFT)
#define IAVFPE_CCQPHIGH1                  0x00009800 /* Reset: VFR */
#define IAVFPE_CCQPHIGH1_PECCQPHIGH_SHIFT 0
#define IAVFPE_CCQPHIGH1_PECCQPHIGH_MASK  IAVF_MASK(0xFFFFFFFF, IAVFPE_CCQPHIGH1_PECCQPHIGH_SHIFT)
#define IAVFPE_CCQPLOW1                 0x0000AC00 /* Reset: VFR */
#define IAVFPE_CCQPLOW1_PECCQPLOW_SHIFT 0
#define IAVFPE_CCQPLOW1_PECCQPLOW_MASK  IAVF_MASK(0xFFFFFFFF, IAVFPE_CCQPLOW1_PECCQPLOW_SHIFT)
#define IAVFPE_CCQPSTATUS1                   0x0000B800 /* Reset: VFR */
#define IAVFPE_CCQPSTATUS1_CCQP_DONE_SHIFT   0
#define IAVFPE_CCQPSTATUS1_CCQP_DONE_MASK    IAVF_MASK(0x1, IAVFPE_CCQPSTATUS1_CCQP_DONE_SHIFT)
#define IAVFPE_CCQPSTATUS1_HMC_PROFILE_SHIFT 4
#define IAVFPE_CCQPSTATUS1_HMC_PROFILE_MASK  IAVF_MASK(0x7, IAVFPE_CCQPSTATUS1_HMC_PROFILE_SHIFT)
#define IAVFPE_CCQPSTATUS1_RDMA_EN_VFS_SHIFT 16
#define IAVFPE_CCQPSTATUS1_RDMA_EN_VFS_MASK  IAVF_MASK(0x3F, IAVFPE_CCQPSTATUS1_RDMA_EN_VFS_SHIFT)
#define IAVFPE_CCQPSTATUS1_CCQP_ERR_SHIFT    31
#define IAVFPE_CCQPSTATUS1_CCQP_ERR_MASK     IAVF_MASK(0x1, IAVFPE_CCQPSTATUS1_CCQP_ERR_SHIFT)
#define IAVFPE_CQACK1              0x0000B000 /* Reset: VFR */
#define IAVFPE_CQACK1_PECQID_SHIFT 0
#define IAVFPE_CQACK1_PECQID_MASK  IAVF_MASK(0x1FFFF, IAVFPE_CQACK1_PECQID_SHIFT)
#define IAVFPE_CQARM1              0x0000B400 /* Reset: VFR */
#define IAVFPE_CQARM1_PECQID_SHIFT 0
#define IAVFPE_CQARM1_PECQID_MASK  IAVF_MASK(0x1FFFF, IAVFPE_CQARM1_PECQID_SHIFT)
#define IAVFPE_CQPDB1              0x0000BC00 /* Reset: VFR */
#define IAVFPE_CQPDB1_WQHEAD_SHIFT 0
#define IAVFPE_CQPDB1_WQHEAD_MASK  IAVF_MASK(0x7FF, IAVFPE_CQPDB1_WQHEAD_SHIFT)
#define IAVFPE_CQPERRCODES1                      0x00009C00 /* Reset: VFR */
#define IAVFPE_CQPERRCODES1_CQP_MINOR_CODE_SHIFT 0
#define IAVFPE_CQPERRCODES1_CQP_MINOR_CODE_MASK  IAVF_MASK(0xFFFF, IAVFPE_CQPERRCODES1_CQP_MINOR_CODE_SHIFT)
#define IAVFPE_CQPERRCODES1_CQP_MAJOR_CODE_SHIFT 16
#define IAVFPE_CQPERRCODES1_CQP_MAJOR_CODE_MASK  IAVF_MASK(0xFFFF, IAVFPE_CQPERRCODES1_CQP_MAJOR_CODE_SHIFT)
#define IAVFPE_CQPTAIL1                  0x0000A000 /* Reset: VFR */
#define IAVFPE_CQPTAIL1_WQTAIL_SHIFT     0
#define IAVFPE_CQPTAIL1_WQTAIL_MASK      IAVF_MASK(0x7FF, IAVFPE_CQPTAIL1_WQTAIL_SHIFT)
#define IAVFPE_CQPTAIL1_CQP_OP_ERR_SHIFT 31
#define IAVFPE_CQPTAIL1_CQP_OP_ERR_MASK  IAVF_MASK(0x1, IAVFPE_CQPTAIL1_CQP_OP_ERR_SHIFT)
#define IAVFPE_IPCONFIG01                        0x00008C00 /* Reset: VFR */
#define IAVFPE_IPCONFIG01_PEIPID_SHIFT           0
#define IAVFPE_IPCONFIG01_PEIPID_MASK            IAVF_MASK(0xFFFF, IAVFPE_IPCONFIG01_PEIPID_SHIFT)
#define IAVFPE_IPCONFIG01_USEENTIREIDRANGE_SHIFT 16
#define IAVFPE_IPCONFIG01_USEENTIREIDRANGE_MASK  IAVF_MASK(0x1, IAVFPE_IPCONFIG01_USEENTIREIDRANGE_SHIFT)
#define IAVFPE_MRTEIDXMASK1                       0x00009000 /* Reset: VFR */
#define IAVFPE_MRTEIDXMASK1_MRTEIDXMASKBITS_SHIFT 0
#define IAVFPE_MRTEIDXMASK1_MRTEIDXMASKBITS_MASK  IAVF_MASK(0x1F, IAVFPE_MRTEIDXMASK1_MRTEIDXMASKBITS_SHIFT)
#define IAVFPE_RCVUNEXPECTEDERROR1                        0x00009400 /* Reset: VFR */
#define IAVFPE_RCVUNEXPECTEDERROR1_TCP_RX_UNEXP_ERR_SHIFT 0
#define IAVFPE_RCVUNEXPECTEDERROR1_TCP_RX_UNEXP_ERR_MASK  IAVF_MASK(0xFFFFFF, IAVFPE_RCVUNEXPECTEDERROR1_TCP_RX_UNEXP_ERR_SHIFT)
#define IAVFPE_TCPNOWTIMER1               0x0000A800 /* Reset: VFR */
#define IAVFPE_TCPNOWTIMER1_TCP_NOW_SHIFT 0
#define IAVFPE_TCPNOWTIMER1_TCP_NOW_MASK  IAVF_MASK(0xFFFFFFFF, IAVFPE_TCPNOWTIMER1_TCP_NOW_SHIFT)
#define IAVFPE_WQEALLOC1                      0x0000C000 /* Reset: VFR */
#define IAVFPE_WQEALLOC1_PEQPID_SHIFT         0
#define IAVFPE_WQEALLOC1_PEQPID_MASK          IAVF_MASK(0x3FFFF, IAVFPE_WQEALLOC1_PEQPID_SHIFT)
#define IAVFPE_WQEALLOC1_WQE_DESC_INDEX_SHIFT 20
#define IAVFPE_WQEALLOC1_WQE_DESC_INDEX_MASK  IAVF_MASK(0xFFF, IAVFPE_WQEALLOC1_WQE_DESC_INDEX_SHIFT)

#endif /* _IAVF_REGISTER_H_ */
