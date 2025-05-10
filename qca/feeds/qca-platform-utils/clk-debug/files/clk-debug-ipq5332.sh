: '
/*
 * Copyright (c) 2019, 2021, 2022  Qualcomm Technologies, Inc.
 *
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */
'

#!/bin/ash

GCC_BLSP1_AHB_CBCR=0x1801008
GCC_BLSP1_SLEEP_CBCR=0x1801010
GCC_BLSP1_QUP1_SPI_APPS_CBCR=0x1802020
GCC_BLSP1_QUP1_I2C_APPS_CBCR=0x1802024
GCC_BLSP1_UART1_APPS_CBCR=0x1802040
GCC_BLSP1_QUP2_SPI_APPS_CBCR=0x1803020
GCC_BLSP1_QUP2_I2C_APPS_CBCR=0x1803024
GCC_BLSP1_UART2_APPS_CBCR=0x1803040
GCC_BLSP1_QUP3_SPI_APPS_CBCR=0x1804020
GCC_BLSP1_QUP3_I2C_APPS_CBCR=0x1804024
GCC_BLSP1_UART3_APPS_CBCR=0x1804054
GCC_TIC_CBCR=0x1805000
GCC_DEBUG_CBCR=0x1805004
GCC_GP1_CBCR=0x1808018
GCC_GP2_CBCR=0x1809018
GCC_IMEM_AXI_CBCR=0x180E008
GCC_IMEM_CFG_AHB_CBCR=0x180E010
GCC_TME_CBCR=0x18100B4
GCC_TME_TS_CBCR=0x18100C0
GCC_TME_SLOW_CBCR=0x18100D0
GCC_TME_RTC_TOGGLE_CBCR=0x18100D8
GCC_MDIO_MASTER_AHB_CBCR=0x1812004
GCC_MDIO_SLAVE_AHB_CBCR=0x181200C
GCC_PRNG_AHB_CBCR=0x1813024
GCC_BOOT_ROM_AHB_CBCR=0x181302C
GCC_UNIPHY0_SYS_CBCR=0x181600C
GCC_UNIPHY0_AHB_CBCR=0x1816010
GCC_UNIPHY1_SYS_CBCR=0x1816018
GCC_UNIPHY1_AHB_CBCR=0x181601C
GCC_NSSNOC_ATB_CBCR=0x1817014
GCC_NSS_TS_CBCR=0x1817018
GCC_NSSNOC_QOSGEN_REF_CBCR=0x181701C
GCC_NSSNOC_TIMEOUT_REF_CBCR=0x1817020
GCC_NSSNOC_SNOC_CBCR=0x1817028
GCC_NSSCFG_CBCR=0x181702C
GCC_NSSNOC_NSSCC_CBCR=0x1817030
GCC_NSSCC_CBCR=0x1817034
GCC_NSSNOC_XO_DCD_CBCR=0x1817074
GCC_NSSNOC_SNOC_1_CBCR=0x181707C
GCC_NSSNOC_PCNOC_1_CBCR=0x1817080
GCC_SNOC_NSSNOC_CBCR=0x1817084
GCC_SNOC_NSSNOC_1_CBCR=0x1817090
GCC_DDRSS_ATB_CBCR=0x1819004
GCC_DDRSS_AHB_CBCR=0x1819008
GCC_MEM_NOC_AHB_CBCR=0x181900C
GCC_MEM_NOC_Q6_AXI_CBCR=0x1819010
GCC_MEM_NOC_SNOC_AXI_CBCR=0x1819018
GCC_MEM_NOC_APSS_AXI_CBCR=0x181901C
GCC_MEM_NOC_QOSGEN_EXTREF_CBCR=0x1819024
GCC_MEM_NOC_TS_CBCR=0x1819028
GCC_MEM_NOC_XO_DBG_CBCR=0x1819038
GCC_TME_BUS_CBCR=0x181A014
GCC_SEC_CTRL_ACC_CBCR=0x181A018
GCC_SEC_CTRL_CBCR=0x181A020
GCC_SEC_CTRL_SENSE_CBCR=0x181A028
GCC_SEC_CTRL_AHB_CBCR=0x181A02C
GCC_SEC_CTRL_BOOT_ROM_PATCH_CBCR=0x181A030
GCC_ADSS_PWM_CBCR=0x181C00C
GCC_CMN_LDO0_AHB_CBCR=0x181D004
GCC_CMN_LDO1_AHB_CBCR=0x181D00C
GCC_CMN_LDO0_SYS_CBCR=0x181D010
GCC_TME_ATB_CBCR=0x181E030
GCC_TME_DBGAPB_CBCR=0x181E034
GCC_TME_DEBUG_CBCR=0x181E038
GCC_TME_AT_CBCR=0x181E03C
GCC_TME_APB_CBCR=0x181E040
GCC_TME_DMI_DBG_HS_CBCR=0x181E044
GCC_APSS_AHB_CBCR=0x1824018
GCC_APSS_AXI_CBCR=0x182401C
GCC_Q6_AXIM_CBCR=0x182500C
GCC_Q6_AXIS_CBCR=0x1825010
GCC_Q6_AHB_CBCR=0x1825014
GCC_Q6_AHB_S_CBCR=0x1825018
GCC_Q6SS_ATBM_CBCR=0x182501C
GCC_Q6_TSCTR_1TO2_CBCR=0x1825020
GCC_Q6SS_PCLKDBG_CBCR=0x1825024
GCC_WCSS_DBG_IFC_APB_CBCR=0x1825038
GCC_WCSS_DBG_IFC_ATB_CBCR=0x182503C
GCC_WCSS_DBG_IFC_NTS_CBCR=0x1825040
GCC_WCSS_DBG_IFC_APB_BDG_CBCR=0x1825048
GCC_WCSS_DBG_IFC_ATB_BDG_CBCR=0x182504C
GCC_WCSS_DBG_IFC_NTS_BDG_CBCR=0x1825050
GCC_WCSS_ECAHB_CBCR=0x1825058
GCC_WCSS_AXIM_CBCR=0x182505C
GCC_WCSS_AXIS_CBCR=0x1825060
GCC_CE_AXI_CBCR=0x1825068
GCC_CE_PCNOC_AHB_CBCR=0x1825070
GCC_CE_AHB_CBCR=0x1825074
GCC_EDPD_AXI_CBCR=0x1825078
GCC_EDPD_AHB_CBCR=0x1825080
GCC_Q6SS_TRIG_CBCR=0x18250A0
GCC_MSG_RAM_AHB_CBCR=0x1826004
GCC_LPASS_SWAY_CBCR=0x1827014
GCC_LPASS_CORE_AXIM_CBCR=0x1827018
GCC_PCIE3X2_AHB_CBCR=0x1828030
GCC_PCIE3X2_AXI_M_CBCR=0x1828038
GCC_PCIE3X2_AXI_S_CBCR=0x1828040
GCC_PCIE3X2_AXI_S_BRIDGE_CBCR=0x1828048
GCC_PCIE3X2_PIPE_CBCR=0x1828068
GCC_PCIE3X2_AUX_CBCR=0x1828070
GCC_PCIE3X2_PHY_AHB_CBCR=0x1828080
GCC_PCIE3X1_0_AHB_CBCR=0x1829030
GCC_PCIE3X1_0_AXI_M_CBCR=0x1829038
GCC_PCIE3X1_0_AXI_S_CBCR=0x1829040
GCC_PCIE3X1_0_AXI_S_BRIDGE_CBCR=0x1829048
GCC_PCIE3X1_0_PIPE_CBCR=0x1829068
GCC_PCIE3X1_0_AUX_CBCR=0x1829070
GCC_PCIE3X1_PHY_AHB_CBCR=0x1829078
GCC_PCIE3X1_1_AHB_CBCR=0x182A00C
GCC_PCIE3X1_1_AXI_M_CBCR=0x182A014
GCC_PCIE3X1_1_AXI_S_CBCR=0x182A01C
GCC_PCIE3X1_1_AXI_S_BRIDGE_CBCR=0x182A024
GCC_PCIE3X1_1_PIPE_CBCR=0x182A068
GCC_PCIE3X1_1_AUX_CBCR=0x182A070
GCC_RBCPR_CX_CBCR=0x182B00C
GCC_RBCPR_CX_AHB_CBCR=0x182B010
GCC_USB0_MASTER_CBCR=0x182C048
GCC_USB0_AUX_CBCR=0x182C050
GCC_USB0_MOCK_UTMI_CBCR=0x182C054
GCC_USB0_SLEEP_CBCR=0x182C058
GCC_USB0_PHY_CFG_AHB_CBCR=0x182C05C
GCC_USB0_PIPE_CBCR=0x182C078
GCC_USB0_LFPS_CBCR=0x182C090
GCC_QDSS_AT_CBCR=0x182D038
GCC_QDSS_STM_CBCR=0x182D040
GCC_QDSS_TRACECLKIN_CBCR=0x182D044
GCC_QDSS_TSCTR_DIV2_CBCR=0x182D048
GCC_QDSS_TSCTR_DIV3_CBCR=0x182D04C
GCC_QDSS_TSCTR_DIV4_CBCR=0x182D050
GCC_QDSS_TSCTR_DIV8_CBCR=0x182D054
GCC_QDSS_TSCTR_DIV16_CBCR=0x182D058
GCC_QDSS_DAP_CBCR=0x182D05C
GCC_QDSS_APB2JTAG_CBCR=0x182D060
GCC_QDSS_ETR_USB_CBCR=0x182D064
GCC_QDSS_DAP_AHB_CBCR=0x182D068
GCC_QDSS_CFG_AHB_CBCR=0x182D06C
GCC_QDSS_EUD_AT_CBCR=0x182D070
GCC_QDSS_TS_CBCR=0x182D078
GCC_SYS_NOC_AXI_CBCR=0x182E01C
GCC_SNOC_QOSGEN_EXTREF_CBCR=0x182E020
GCC_SNOC_LPASS_CFG_CBCR=0x182E028
GCC_SYS_NOC_APSS_AHB_CBCR=0x182E02C
GCC_SYS_NOC_WCSS_AHB_CBCR=0x182E030
GCC_SYS_NOC_QDSS_STM_AXI_CBCR=0x182E034
GCC_SYS_NOC_AT_CBCR=0x182E038
GCC_SNOC_PCNOC_AHB_CBCR=0x182E03C
GCC_SNOC_TME_CFG_CBCR=0x182E040
GCC_SNOC_PCIE3_2LANE_S_CBCR=0x182E048
GCC_SNOC_PCIE3_1LANE_S_CBCR=0x182E04C
GCC_SNOC_PCIE3_1LANE_1_M_CBCR=0x182E050
GCC_SNOC_USB_CBCR=0x182E058
GCC_SNOC_TME_CBCR=0x182E05C
GCC_SNOC_XO_DCD_CBCR=0x182E060
GCC_SNOC_TS_CBCR=0x182E068
GCC_SNOC_PCIE3_2LANE_M_CBCR=0x182E07C
GCC_SNOC_PCIE3_1LANE_M_CBCR=0x182E080
GCC_SNOC_XO_DBG_CBCR=0x182E0A0
GCC_SNOC_PCIE3_1LANE_1_S_CBCR=0x182E0AC
GCC_WCSS_MST_ASYNC_BDG_CBCR=0x182E0B0
GCC_WCSS_SLV_ASYNC_BDG_CBCR=0x182E0B4
GCC_USB0_EUD_AT_CBCR=0x1830004
GCC_PCNOC_AHB_CBCR=0x1831018
GCC_PCNOC_TIC_CBCR=0x183101C
GCC_PCNOC_LPASS_CBCR=0x1831020
GCC_PCNOC_AT_CBCR=0x1831024
GCC_PCNOC_XO_DCD_CBCR=0x1831028
GCC_PCNOC_TS_CBCR=0x183102C
GCC_PCNOC_BUS_TIMEOUT0_AHB_CBCR=0x1831034
GCC_PCNOC_BUS_TIMEOUT1_AHB_CBCR=0x183103C
GCC_PCNOC_BUS_TIMEOUT2_AHB_CBCR=0x1831044
GCC_PCNOC_BUS_TIMEOUT3_AHB_CBCR=0x183104C
GCC_PCNOC_BUS_TIMEOUT4_AHB_CBCR=0x1831054
GCC_PCNOC_BUS_TIMEOUT5_AHB_CBCR=0x183105C
GCC_PCNOC_BUS_TIMEOUT6_AHB_CBCR=0x1831064
GCC_PCNOC_BUS_TIMEOUT7_AHB_CBCR=0x183106C
GCC_PCNOC_BUS_TIMEOUT8_AHB_CBCR=0x1831074
GCC_PCNOC_BUS_TIMEOUT9_AHB_CBCR=0x183107C
GCC_PCNOC_DCC_CBCR=0x1831080
GCC_PCNOC_XO_DBG_CBCR=0x1831084
GCC_QPIC_IO_MACRO_CBCR=0x183200C
GCC_QPIC_AHB_CBCR=0x1832010
GCC_QPIC_CBCR=0x1832014
GCC_QPIC_SLEEP_CBCR=0x183201C
GCC_SDCC1_APPS_CBCR=0x183302C
GCC_SDCC1_AHB_CBCR=0x1833034
GCC_XO_CBCR=0x1834018
GCC_XO_DIV4_CBCR=0x183401C
GCC_IM_SLEEP_CBCR=0x1834020
GCC_AHB_CBCR=0x1834024
GCC_DCC_CBCR=0x1835004
GCC_DCC_XO_CBCR=0x183500C
GCC_SPDM_PCNOC_CY_CBCR=0x1836008
GCC_SPDM_SNOC_CY_CBCR=0x1836010
GCC_SPDM_DEBUG_CY_CBCR=0x1836014
GCC_SPDM_FF_CBCR=0x1836018
GCC_MPM_AHB_CBCR=0x1837004
GCC_APC0_VOLTAGE_DROOP_DETECTOR_GPLL0_CBCR=0x183800C
GCC_CMN_12GPLL_AHB_CBCR=0x183A004
GCC_CMN_12GPLL_SYS_CBCR=0x183A008
GCC_CMN_12GPLL_APU_CBCR=0x183A00C
GCC_TCSR_AHB_CBCR=0x183D004
GCC_TLMM_AHB_CBCR=0x183E004
GCC_TLMM_CBCR=0x183E008

function calculate_clk(){
GCC_GCC_DEBUG_CLK_CTL=0x183F010
GCC_XO_DIV4_CBCR=0x183401C
GCC_CLOCK_FRQ_MEASURE_CTL=0x183F014
GCC_CLOCK_FRQ_MEASURE_STATUS=0x183F018
GCC_XO_CBCR=0x1834018
FREQ_XO_DIV4=6
APCS_CLK_MUX_SEL=244
Bit16=65536
Bit20=1048576
Bit25=33554432
Bit31=2147483648
RandomCnt1=67584
RandomCnt2=67618
Constant1pt5=1.5
Constant3pt5=3.5
post_div=12288
multiplier=4
bcalculateclk=0

#Reading user input for clock selection
#echo Provide the Mux selection for DEBUG_CLK_CTL

ClockSel=$2
#echo entered clock $ClockSel

#Select clock mux and write on memory
var1=$(expr "$ClockSel" + "$Bit16" + "$post_div")
devmem $GCC_GCC_DEBUG_CLK_CTL 32 $var1
#echo mux $var1

#Disable Freq Measure control
devmem $GCC_CLOCK_FRQ_MEASURE_CTL 32 0x0

#Write Freq Measure control with random count with Bit20 set
devmem $GCC_CLOCK_FRQ_MEASURE_CTL 32 $(expr "$RandomCnt1" + "$Bit20")

#Read Frq measure status
var1=$(devmem $GCC_CLOCK_FRQ_MEASURE_STATUS)
#echo Freq Measure Status First Cnt : $var1
#Convert the var1 to decimal
var1=$(($var1))

#Check the Bit25 for counter terminates or not
while [ $var1 -lt $Bit25 ]
do
var1=$(devmem $GCC_CLOCK_FRQ_MEASURE_STATUS)
#echo Freq Measure Status First Cnt : $var1
#convert the value to decimal
var1=$(($var1))
done

#Get the CBCR Register name
cbcrname=$1
#echo cbcr $cbcrname

cbcrstring=CBCR

cbcrname=${cbcrname/CLK/$cbcrstring}

#Get the Address
eval "cbcraddress=\${$cbcrname}"
#echo cbcraddress $cbcraddress
#print clock name
printf "%s""$1";

#Check for CBCR register is present or not
if [ "$(($cbcraddress))" == 0 ]
then
#No CBCR register is present for this clock, check for register differences
#Disable Freq Measure control
devmem $GCC_CLOCK_FRQ_MEASURE_CTL 32 0x0

#Load again Freq Measure control with next random count with Bit20 set
devmem $GCC_CLOCK_FRQ_MEASURE_CTL 32 $(expr "$RandomCnt2" + "$Bit20")

#Read Frq measure status
var2=$(devmem $GCC_CLOCK_FRQ_MEASURE_STATUS)
#echo Freq Measure Status Second Cnt : $var2
#Convert the var2 to decimal
var2=$(($var2))

#Check the Bit25 for counter terminates or not
while [ $var2 -lt $Bit25 ]
do
var2=$(devmem $GCC_CLOCK_FRQ_MEASURE_STATUS)
#echo Freq Measure Status Second Cnt : $var2
#convert the value to decimal
var2=$(($var2))
done

if [ "$var1" == "$var2" ]
then
printf "\t\t\t""OFF \t\tnone";
else
bcalculateclk=1
printf "\t\t\t""ON \t\tnone";
fi

else
#CBCR register is present for this clock, read CBCR register
clkstate=$(devmem $cbcraddress)
#check Clock is enabled or not
if [ "$(($clkstate))" -ge "$Bit31" ]
then
printf "\t\t\t""OFF \t\tcbcr";
else
#Set clock read to ONE
bcalculateclk=1
printf "\t\t\t""ON \t\tcbcr";
fi
fi
if [ "$bcalculateclk" == 1 ]
then
#Eliminate the Bit25 in Frq Measure status
var1=$(expr "$var1" - "$Bit25")

#Measure_CNT+1.5
var2=`echo - | awk '{print "'"$var1"'" + "'"$Constant1pt5"'" }'`

#XO_DIV4_TERM_CNT+3.5
var3=`echo - | awk '{print "'"$RandomCnt1"'" + "'"$Constant3pt5"'" }'`

#Freq(XO_DIV4)*var2/var3
var1=`echo - | awk '{print "'"$FREQ_XO_DIV4"'" *  "'"$var2"'" }'`
var2=`echo - | awk '{print "'"$var1"'" /  "'"$var3"'" }'`

if [ $ClockSel -ne $APP_CLK_SEL ]
then
#Apply multiplier
var2=`echo - | awk '{print "'"$var2"'" *  "'"$multiplier"'" }'`
else
#Apply multiplier for APCS Cluster clk
#APCS freq generated via applied formula is half of system freq
#APCS multiplier is *2 to get the final freq value
apcs_multiplier=64
var2=`echo - | awk '{print "'"$var2"'" *  "'"$apcs_multiplier"'" }'`
fi

#convert to Hz
var3=`echo - | awk '{print "'"$var2"'" * "'"1000000"'" }'`
printf "\t%12.0f" "$var3";

else
printf "\t----------";
fi
printf "\r\n"

}

inputstr=$1
if [[ "$inputstr" == HELP || "$inputstr" == help ]]
then
echo "Execute the script and grep for desired register string"
exit 0
fi

echo -e "   Clock Name   \t State \t CBCR Type \t Frequency(Hz)"
echo "--------------------------------------------------------------------------"

calculate_clk	gpll0_out_test_clk	0
calculate_clk	gpll2_out_test_clk	1
calculate_clk	gpll4_out_test_clk	2
calculate_clk	gpll6_out_test_clk	3
calculate_clk	ddr_phy_pll_test_clk	4
calculate_clk	apss0_gcc_pll_test_clk	5
calculate_clk	wcss_zinc_pll_test_clk	6
calculate_clk	lpass_pllout_lv_test_clk	7
calculate_clk	q6ss_pll_test_clk	8
calculate_clk	gpll0_dtest	9
calculate_clk	gpll2_dtest	10
calculate_clk	gpll4_dtest	11
calculate_clk	gpll6_dtest	12
calculate_clk	gcc_snoc_nssnoc_clk	13
calculate_clk	gcc_snoc_nssnoc_1_clk	14
calculate_clk	gcc_pcie3x1_1_ahb_clk	15
calculate_clk	gcc_pcie3x1_1_aux_clk	16
calculate_clk	gcc_pcie3x1_1_axi_m_clk	17
calculate_clk	gcc_pcie3x1_1_axi_s_clk	18
calculate_clk	gcc_pcie3x1_1_axi_s_bridge_clk	19
calculate_clk	gcc_snoc_pcie3_1lane_1_s_clk	20
calculate_clk	gcc_gpll0_out_main_div2_cgc_clk	21
calculate_clk	gcc_gpll0_out_main_cgc_clk	22
calculate_clk	gcc_gpll2_out_main_cgc_clk	23
calculate_clk	gcc_gpll4_out_main_cgc_clk	24
calculate_clk	gcc_gpll6_out_main_cgc_clk	25
calculate_clk	gcc_apss_gpll0_clk_src_clk	26
calculate_clk	gcc_apss_gpll2_clk_src_clk	27
calculate_clk	gcc_apss_gpll4_clk_src_clk	28
calculate_clk	gcc_apss_ahb_clk	29
calculate_clk	gcc_apss_axi_clk	30
calculate_clk	gcc_q6_axim_clk	31
calculate_clk	gcc_q6_axis_clk	32
calculate_clk	gcc_q6_ahb_clk	33
calculate_clk	gcc_q6_ahb_s_clk	34
calculate_clk	gcc_q6ss_atbm_clk	35
calculate_clk	gcc_q6_tsctr_1to2_clk	36
calculate_clk	gcc_q6ss_pclkdbg_clk	37
calculate_clk	gcc_q6ss_trig_clk	38
calculate_clk	gcc_pcie3x1_phy_ahb_clk	39
calculate_clk	gcc_wcss_dbg_ifc_apb_clk	40
calculate_clk	gcc_wcss_dbg_ifc_atb_clk	41
calculate_clk	gcc_wcss_dbg_ifc_nts_clk	42
calculate_clk	gcc_wcss_axim_clk	43
calculate_clk	gcc_wcss_dbg_ifc_apb_bdg_clk	44
calculate_clk	gcc_wcss_dbg_ifc_atb_bdg_clk	45
calculate_clk	gcc_wcss_dbg_ifc_nts_bdg_clk	46
calculate_clk	gcc_wcss_axis_clk	47
calculate_clk	gcc_wcss_ecahb_clk	48
calculate_clk	gcc_pcie3x2_phy_ahb_clk	49
calculate_clk	gcc_wcss_mst_async_bdg_clk	50
calculate_clk	gcc_wcss_slv_async_bdg_clk	51
calculate_clk	gcc_ce_axi_clk	53
calculate_clk	gcc_ce_pcnoc_ahb_clk	54
calculate_clk	gcc_ce_ahb_clk	55
calculate_clk	gcc_edpd_axi_clk	56
calculate_clk	gcc_edpd_ahb_clk	57
calculate_clk	gcc_nssnoc_atb_clk	58
calculate_clk	gcc_nss_ts_clk	59
calculate_clk	gcc_nssnoc_qosgen_ref_clk	60
calculate_clk	gcc_nssnoc_timeout_ref_clk	61
calculate_clk	gcc_nssnoc_snoc_clk	62
calculate_clk	gcc_nsscfg_clk	63
calculate_clk	gcc_nssnoc_nsscc_clk	64
calculate_clk	gcc_nsscc_clk	65
calculate_clk	gcc_nssnoc_xo_dcd_clk	66
calculate_clk	gcc_nssnoc_snoc_1_clk	67
calculate_clk	gcc_nssnoc_pcnoc_1_clk	68
calculate_clk	gcc_lpass_sway_clk	69
calculate_clk	gcc_lpass_core_axim_clk	70
calculate_clk	gcc_pcie3x2_ahb_clk	71
calculate_clk	gcc_pcie3x2_aux_clk	72
calculate_clk	gcc_pcie3x2_axi_m_clk	73
calculate_clk	gcc_pcie3x2_axi_s_clk	74
calculate_clk	gcc_pcie3x2_axi_s_bridge_clk	75
calculate_clk	gcc_pcie3x1_0_ahb_clk	76
calculate_clk	gcc_pcie3x1_0_aux_clk	77
calculate_clk	gcc_pcie3x1_0_axi_m_clk	78
calculate_clk	gcc_pcie3x1_0_axi_s_clk	79
calculate_clk	gcc_pcie3x1_0_axi_s_bridge_clk	80
calculate_clk	gcc_usb0_master_clk	81
calculate_clk	gcc_usb0_aux_clk	82
calculate_clk	gcc_usb0_mock_utmi_clk	83
calculate_clk	gcc_usb0_sleep_clk	84
calculate_clk	gcc_usb0_phy_cfg_ahb_clk	85
calculate_clk	gcc_usb0_eud_at_clk	86
calculate_clk	gcc_qdss_at_clk	87
calculate_clk	gcc_qdss_stm_clk	88
calculate_clk	gcc_qdss_traceclkin_clk	89
calculate_clk	gcc_qdss_tsctr_div2_clk	90
calculate_clk	gcc_qdss_tsctr_div3_clk	91
calculate_clk	gcc_qdss_tsctr_div4_clk	92
calculate_clk	gcc_qdss_tsctr_div8_clk	93
calculate_clk	gcc_qdss_tsctr_div16_clk	94
calculate_clk	gcc_qdss_dap_clk	95
calculate_clk	gcc_qdss_apb2jtag_clk	96
calculate_clk	gcc_qdss_etr_usb_clk	97
calculate_clk	gcc_qdss_dap_ahb_clk	98
calculate_clk	gcc_qdss_cfg_ahb_clk	99
calculate_clk	gcc_qdss_eud_at_clk	100
calculate_clk	gcc_qdss_ts_clk	101
calculate_clk	gcc_sys_noc_axi_clk	102
calculate_clk	gcc_snoc_qosgen_extref_clk	103
calculate_clk	gcc_snoc_lpass_cfg_clk	104
calculate_clk	gcc_sys_noc_apss_ahb_clk	105
calculate_clk	gcc_sys_noc_wcss_ahb_clk	106
calculate_clk	gcc_sys_noc_qdss_stm_axi_clk	107
calculate_clk	gcc_sys_noc_at_clk	108
calculate_clk	gcc_snoc_pcnoc_ahb_clk	109
calculate_clk	gcc_snoc_tme_cfg_clk	110
calculate_clk	gcc_snoc_pcie3_2lane_s_clk	111
calculate_clk	gcc_snoc_pcie3_1lane_s_clk	112
calculate_clk	gcc_snoc_usb_clk	113
calculate_clk	gcc_snoc_tme_clk	114
calculate_clk	gcc_snoc_xo_dcd_clk	115
calculate_clk	gcc_snoc_ts_clk	116
calculate_clk	gcc_snoc_xo_dbg_clk	117
calculate_clk	gcc_snoc_pcie3_2lane_m_clk	118
calculate_clk	gcc_snoc_pcie3_1lane_m_clk	119
calculate_clk	gcc_rbcpr_cx_clk	120
calculate_clk	gcc_rbcpr_cx_ahb_clk	121
calculate_clk	gcc_pcnoc_ahb_clk	122
calculate_clk	gcc_pcnoc_tic_clk	123
calculate_clk	gcc_pcnoc_lpass_clk	124
calculate_clk	gcc_pcnoc_at_clk	125
calculate_clk	gcc_pcnoc_xo_dcd_clk	126
calculate_clk	gcc_pcnoc_ts_clk	127
calculate_clk	gcc_pcnoc_xo_dbg_clk	128
calculate_clk	gcc_pcnoc_dcc_clk	129
calculate_clk	gcc_tme_clk	130
calculate_clk	gcc_tme_ts_clk	131
calculate_clk	gcc_tme_slow_clk	132
calculate_clk	gcc_tme_dmi_dbg_hs_clk	133
calculate_clk	gcc_tme_rtc_toggle_clk	134
calculate_clk	gcc_tme_atb_clk	135
calculate_clk	gcc_tme_dbgapb_clk	136
calculate_clk	gcc_tme_debug_clk	137
calculate_clk	gcc_tme_at_clk	138
calculate_clk	gcc_tme_apb_clk	139
calculate_clk	gcc_tme_bus_clk	140
calculate_clk	gcc_qpic_io_macro_clk	141
calculate_clk	gcc_qpic_ahb_clk	142
calculate_clk	gcc_qpic_clk	143
calculate_clk	gcc_qpic_sleep_clk	144
calculate_clk	gcc_blsp1_ahb_clk	145
calculate_clk	gcc_blsp1_sleep_clk	146
calculate_clk	gcc_blsp1_qup1_spi_apps_clk	147
calculate_clk	gcc_blsp1_qup1_i2c_apps_clk	148
calculate_clk	gcc_blsp1_uart1_apps_clk	149
calculate_clk	gcc_blsp1_qup2_spi_apps_clk	150
calculate_clk	gcc_blsp1_qup2_i2c_apps_clk	151
calculate_clk	gcc_blsp1_uart2_apps_clk	152
calculate_clk	gcc_blsp1_qup3_spi_apps_clk	153
calculate_clk	gcc_blsp1_qup3_i2c_apps_clk	154
calculate_clk	gcc_blsp1_uart3_apps_clk	155
calculate_clk	gcc_sdcc1_apps_clk	156
calculate_clk	gcc_sdcc1_ahb_clk	157
calculate_clk	gcc_xo_clk	158
calculate_clk	gcc_xo_div4_clk	159
calculate_clk	gcc_im_sleep_clk	160
calculate_clk	gcc_ahb_clk	161
calculate_clk	gcc_sec_ctrl_acc_clk	162
calculate_clk	gcc_sec_ctrl_clk	163
calculate_clk	gcc_sec_ctrl_sense_clk	164
calculate_clk	gcc_sec_ctrl_ahb_clk	165
calculate_clk	gcc_sec_ctrl_boot_rom_patch_clk	166
calculate_clk	gcc_mdio_master_ahb_clk	167
calculate_clk	gcc_mdio_slave_ahb_clk	168
calculate_clk	gcc_tcsr_ahb_clk	169
calculate_clk	gcc_ddrss_atb_clk	170
calculate_clk	gcc_ddrss_ahb_clk	171
calculate_clk	gcc_mem_noc_ahb_clk	172
calculate_clk	gcc_mem_noc_q6_axi_clk	173
calculate_clk	gcc_snoc_pcie3_1lane_1_m_clk	174
calculate_clk	gcc_mem_noc_snoc_axi_clk	175
calculate_clk	gcc_mem_noc_apss_axi_clk	176
calculate_clk	tme_clk_mux_out	177
calculate_clk	gcc_mem_noc_qosgen_extref_clk	178
calculate_clk	gcc_mem_noc_ts_clk	179
calculate_clk	gcc_mem_noc_xo_dbg_clk	180
calculate_clk	gcc_adss_pwm_clk	181
calculate_clk	gcc_tlmm_ahb_clk	182
calculate_clk	gcc_tlmm_clk	183
calculate_clk	gcc_msg_ram_ahb_clk	184
calculate_clk	gcc_tic_clk	185
calculate_clk	gcc_spdm_pcnoc_cy_clk	186
calculate_clk	gcc_spdm_snoc_cy_clk	187
calculate_clk	gcc_cmn_ldo0_sys_clk	188
calculate_clk	gcc_spdm_ff_clk	189
calculate_clk	gcc_prng_ahb_clk	190
calculate_clk	gcc_boot_rom_ahb_clk	191
calculate_clk	gcc_imem_axi_clk	192
calculate_clk	gcc_imem_cfg_ahb_clk	193
calculate_clk	gcc_uniphy0_sys_clk	194
calculate_clk	gcc_uniphy0_ahb_clk	195
calculate_clk	gcc_uniphy1_sys_clk	196
calculate_clk	gcc_uniphy1_ahb_clk	197
calculate_clk	gcc_cmn_12gpll_ahb_clk	200
calculate_clk	gcc_cmn_12gpll_sys_clk	201
calculate_clk	gcc_cmn_12gpll_apu_clk	202
calculate_clk	gcc_cmn_ldo0_ahb_clk	203
calculate_clk	gcc_cmn_ldo1_ahb_clk	204
calculate_clk	gcc_mpm_ahb_clk	205
calculate_clk	gcc_usb0_lfps_clk	206
calculate_clk	gcc_pcnoc_bus_timeout0_ahb_clk	210
calculate_clk	gcc_pcnoc_bus_timeout1_ahb_clk	211
calculate_clk	gcc_pcnoc_bus_timeout2_ahb_clk	212
calculate_clk	gcc_pcnoc_bus_timeout3_ahb_clk	213
calculate_clk	gcc_pcnoc_bus_timeout4_ahb_clk	214
calculate_clk	gcc_pcnoc_bus_timeout5_ahb_clk	215
calculate_clk	gcc_pcnoc_bus_timeout6_ahb_clk	216
calculate_clk	gcc_pcnoc_bus_timeout7_ahb_clk	217
calculate_clk	gcc_pcnoc_bus_timeout8_ahb_clk	218
calculate_clk	gcc_pcnoc_bus_timeout9_ahb_clk	219
calculate_clk	gcc_gp1_clk	220
calculate_clk	gcc_gp2_clk	221
calculate_clk	gcc_gp3_clk	222
calculate_clk	gcc_dcc_clk	223
calculate_clk	gcc_dcc_xo_clk	224
calculate_clk	gcc_apc0_voltage_droop_detector_gpll0_clk	241
calculate_clk	usb30_utmi_clk	242
calculate_clk	usb0_hspw_clk_480m_test	243
calculate_clk	apss_gcc_dbg_clk	244
calculate_clk	mpm_gcc_temp_sensor_ringosc_clk	245
calculate_clk	usb_pcie_wrapper_pipe_clk	246
calculate_clk	pcie_2lane_phy_pipe_clk	247
calculate_clk	pcie_2lane_phy_pipe_clk_x1	248
calculate_clk	lpass_dbg_clk	251
calculate_clk	q6ss_pll_dtest	252
calculate_clk	q6ss_dbg_clk	253
calculate_clk	wcss_gcc_debug_clk_obs	254
calculate_clk	wcss_zinc_dbg_out_clk	255
calculate_clk	ddrc_pll_test_clk	256
calculate_clk	nss_cc_debug_clk	257
calculate_clk	q6ss_core_clk_div8_mon	261
calculate_clk	gcc_pcie3x1_0_rchg_clk	274
calculate_clk	gcc_pcie3x1_1_rchg_clk	275
calculate_clk	gcc_pcie3x2_rchg_clk	276
