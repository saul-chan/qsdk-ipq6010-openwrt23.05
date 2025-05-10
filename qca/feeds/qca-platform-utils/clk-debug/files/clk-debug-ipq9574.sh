: '
/*
 * Copyright (c) 2019, 2021 Qualcomm Technologies, Inc.
 *
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */
'

#!/bin/ash

GCC_BLSP1_AHB_CBCR=0x1801004
GCC_BLSP1_SLEEP_CBCR=0x1801008
GCC_BLSP1_QUP1_SPI_APPS_CBCR=0x1802020
GCC_BLSP1_QUP1_I2C_APPS_CBCR=0x1802024
GCC_BLSP1_UART1_APPS_CBCR=0x1802040
GCC_BLSP1_QUP2_SPI_APPS_CBCR=0x1803020
GCC_BLSP1_QUP2_I2C_APPS_CBCR=0x1803024
GCC_BLSP1_UART2_APPS_CBCR=0x1803040
GCC_BLSP1_QUP3_SPI_APPS_CBCR=0x1804020
GCC_BLSP1_QUP3_I2C_APPS_CBCR=0x1804024
GCC_BLSP1_UART3_APPS_CBCR=0x1804054
GCC_BLSP1_QUP4_SPI_APPS_CBCR=0x1805020
GCC_BLSP1_QUP4_I2C_APPS_CBCR=0x1805024
GCC_BLSP1_UART4_APPS_CBCR=0x1805040
GCC_BLSP1_QUP5_SPI_APPS_CBCR=0x1806020
GCC_BLSP1_QUP5_I2C_APPS_CBCR=0x1806024
GCC_BLSP1_UART5_APPS_CBCR=0x1806040
GCC_BLSP1_QUP6_SPI_APPS_CBCR=0x1807020
GCC_BLSP1_QUP6_I2C_APPS_CBCR=0x1807024
GCC_BLSP1_UART6_APPS_CBCR=0x1807040
GCC_GP1_CBCR=0x1808018
GCC_GP2_CBCR=0x1809018
GCC_GP3_CBCR=0x180A018
GCC_IMEM_AXI_CBCR=0x180E004
GCC_IMEM_CFG_AHB_CBCR=0x180E008
GCC_TME_CBCR=0x18100B4
GCC_TME_TS_CBCR=0x18100C0
GCC_TME_SLOW_CBCR=0x18100D0
GCC_TME_RTC_TOGGLE_CBCR=0x18100D8
GCC_APSS_TCU_ASYNC_CBCR=0x1812010
GCC_SMMU_CFG_CBCR=0x1812028
GCC_PCNOC_TBU_CBCR=0x1812030
GCC_ANOC0_TBU_CBCR=0x1812038
GCC_NSS_TBU_CBCR=0x1812040
GCC_ANOC1_TBU_CBCR=0x1812048
GCC_WCSS_Q6_TBU_CBCR=0x1812050
GCC_PRNG_AHB_CBCR=0x1813024
GCC_BOOT_ROM_AHB_CBCR=0x181302C
GCC_CRYPTO_CBCR=0x181600C
GCC_CRYPTO_AXI_CBCR=0x1816010
GCC_CRYPTO_AHB_CBCR=0x1816014
GCC_UBI32_DBG0_CBCR=0x1817010
GCC_NSSNOC_ATB_CBCR=0x1817014
GCC_NSS_TS_CBCR=0x1817018
GCC_NSSNOC_QOSGEN_REF_CBCR=0x181701C
GCC_NSSNOC_TIMEOUT_REF_CBCR=0x1817020
GCC_NSSNOC_MEMNOC_CBCR=0x1817024
GCC_NSSNOC_SNOC_CBCR=0x1817028
GCC_NSSCFG_CBCR=0x181702C
GCC_NSSNOC_NSSCC_CBCR=0x1817030
GCC_NSSCC_CBCR=0x1817034
GCC_MDIO_AHB_CBCR=0x1817040
GCC_UNIPHY0_SYS_CBCR=0x1817048
GCC_UNIPHY0_AHB_CBCR=0x181704C
GCC_UNIPHY1_SYS_CBCR=0x1817058
GCC_UNIPHY1_AHB_CBCR=0x181705C
GCC_UNIPHY2_SYS_CBCR=0x1817068
GCC_UNIPHY2_AHB_CBCR=0x181706C
GCC_NSSNOC_XO_DCD_CBCR=0x1817074
GCC_NSSNOC_SNOC_1_CBCR=0x181707C
GCC_NSSNOC_PCNOC_1_CBCR=0x1817080
GCC_NSSNOC_MEM_NOC_1_CBCR=0x1817084
GCC_DDRSS_ATB_CBCR=0x1819004
GCC_DDRSS_AHB_CBCR=0x1819008
GCC_MEM_NOC_AHB_CBCR=0x181900C
GCC_MEM_NOC_Q6_AXI_CBCR=0x1819010
GCC_MEM_NOC_NSSNOC_CBCR=0x1819014
GCC_MEM_NOC_SNOC_AXI_CBCR=0x1819018
GCC_MEM_NOC_APSS_AXI_CBCR=0x181901C
GCC_MEM_NOC_APSS_TCU_CBCR=0x1819020
GCC_MEM_NOC_QOSGEN_EXTREF_CBCR=0x1819024
GCC_MEM_NOC_TS_CBCR=0x1819028
GCC_DDRSS_SMS_SLOW_CBCR=0x181902C
GCC_MEM_NOC_XO_DBG_CBCR=0x1819038
GCC_TME_BUS_CBCR=0x181A014
GCC_SEC_CTRL_ACC_CBCR=0x181A018
GCC_SEC_CTRL_CBCR=0x181A01C
GCC_SEC_CTRL_SENSE_CBCR=0x181A020
GCC_SEC_CTRL_AHB_CBCR=0x181A024
GCC_SEC_CTRL_BOOT_ROM_PATCH_CBCR=0x181A028
GCC_ADSS_PWM_CBCR=0x181C00C
GCC_TME_ATB_CBCR=0x181E030
GCC_TME_DBGAPB_CBCR=0x181E034
GCC_TME_DEBUG_CBCR=0x181E038
GCC_TME_AT_CBCR=0x181E03C
GCC_TME_APB_CBCR=0x181E040
GCC_TME_DMI_DBG_HS_CBCR=0x181E044
GCC_RPM_PROC_HCLK_CBCR=0x1823010
GCC_RPM_BUS_AHB_CBCR=0x1823014
GCC_RPM_SLEEP_CBCR=0x1823018
GCC_RPM_TIMER_CBCR=0x182301C
GCC_RPM_MST_M2_NOC_AHB_CBCR=0x1823020
GCC_RPM_NOC_MST_AHB_CBCR=0x1823024
GCC_APSS_AHB_CBCR=0x1824018
GCC_APSS_AXI_CBCR=0x182401C
GCC_Q6_AXIM_CBCR=0x182500C
GCC_Q6_AXIM2_CBCR=0x1825010
GCC_Q6_AHB_CBCR=0x1825014
GCC_Q6_AHB_S_CBCR=0x1825018
GCC_Q6SS_ATBM_CBCR=0x182501C
GCC_Q6_TSCTR_1TO2_CBCR=0x1825020
GCC_Q6SS_PCLKDBG_CBCR=0x1825024
GCC_WCSS_DBG_IFC_APB_CBCR=0x1825038
GCC_WCSS_DBG_IFC_ATB_CBCR=0x182503C
GCC_WCSS_DBG_IFC_NTS_CBCR=0x1825040
GCC_WCSS_DBG_IFC_DAPBUS_CBCR=0x1825044
GCC_WCSS_DBG_IFC_APB_BDG_CBCR=0x1825048
GCC_WCSS_DBG_IFC_ATB_BDG_CBCR=0x182504C
GCC_WCSS_DBG_IFC_NTS_BDG_CBCR=0x1825050
GCC_WCSS_DBG_IFC_DAPBUS_BDG_CBCR=0x1825054
GCC_WCSS_ECAHB_CBCR=0x1825058
GCC_WCSS_ACMT_CBCR=0x182505C
GCC_WCSS_AHB_S_CBCR=0x1825060
GCC_WCSS_AXI_M_CBCR=0x1825064
GCC_Q6SS_TRIG_CBCR=0x1825068
GCC_Q6SS_BOOT_CBCR=0x1825080
GCC_MSG_RAM_AHB_CBCR=0x1826004
GCC_LPASS_SWAY_CBCR=0x1827014
GCC_LPASS_CORE_AXIM_CBCR=0x1827018
GCC_PCIE0_AHB_CBCR=0x1828030
GCC_PCIE0_AUX_CBCR=0x1828034
GCC_PCIE0_AXI_M_CBCR=0x1828038
GCC_PCIE0_AXI_S_CBCR=0x182803C
GCC_PCIE0_AXI_S_BRIDGE_CBCR=0x1828040
GCC_PCIE0_PIPE_CBCR=0x1828044
GCC_PCIE1_AHB_CBCR=0x1829030
GCC_PCIE1_AUX_CBCR=0x1829034
GCC_PCIE1_AXI_M_CBCR=0x1829038
GCC_PCIE1_AXI_S_CBCR=0x182903C
GCC_PCIE1_AXI_S_BRIDGE_CBCR=0x1829040
GCC_PCIE1_PIPE_CBCR=0x1829044
GCC_PCIE2_AHB_CBCR=0x182A030
GCC_PCIE2_AUX_CBCR=0x182A034
GCC_PCIE2_AXI_M_CBCR=0x182A038
GCC_PCIE2_AXI_S_CBCR=0x182A03C
GCC_PCIE2_AXI_S_BRIDGE_CBCR=0x182A040
GCC_PCIE2_PIPE_CBCR=0x182A044
GCC_PCIE3_AHB_CBCR=0x182B030
GCC_PCIE3_AUX_CBCR=0x182B034
GCC_PCIE3_AXI_M_CBCR=0x182B038
GCC_PCIE3_AXI_S_CBCR=0x182B03C
GCC_PCIE3_AXI_S_BRIDGE_CBCR=0x182B040
GCC_PCIE3_PIPE_CBCR=0x182B044
GCC_USB0_MASTER_CBCR=0x182C044
GCC_USB0_AUX_CBCR=0x182C048
GCC_USB0_MOCK_UTMI_CBCR=0x182C04C
GCC_USB0_PIPE_CBCR=0x182C054
GCC_USB0_SLEEP_CBCR=0x182C058
GCC_USB0_PHY_CFG_AHB_CBCR=0x182C05C
GCC_QDSS_AT_CBCR=0x182D038
GCC_QDSS_STM_CBCR=0x182D03C
GCC_QDSS_TRACECLKIN_CBCR=0x182D040
GCC_QDSS_TSCTR_DIV2_CBCR=0x182D044
GCC_QDSS_TSCTR_DIV3_CBCR=0x182D048
GCC_QDSS_TSCTR_DIV4_CBCR=0x182D04C
GCC_QDSS_TSCTR_DIV8_CBCR=0x182D050
GCC_QDSS_TSCTR_DIV16_CBCR=0x182D054
GCC_QDSS_DAP_CBCR=0x182D058
GCC_QDSS_APB2JTAG_CBCR=0x182D05C
GCC_QDSS_ETR_USB_CBCR=0x182D060
GCC_QDSS_DAP_AHB_CBCR=0x182D064
GCC_QDSS_CFG_AHB_CBCR=0x182D068
GCC_QDSS_EUD_AT_CBCR=0x182D06C
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
GCC_SNOC_PCIE0_1LANE_S_CBCR=0x182E048
GCC_SNOC_PCIE1_1LANE_S_CBCR=0x182E04C
GCC_SNOC_PCIE2_2LANE_S_CBCR=0x182E050
GCC_SNOC_PCIE3_2LANE_S_CBCR=0x182E054
GCC_SNOC_USB_CBCR=0x182E058
GCC_SNOC_TME_CBCR=0x182E05C
GCC_SNOC_XO_DCD_CBCR=0x182E060
GCC_SNOC_TS_CBCR=0x182E068
GCC_ANOC0_AXI_CBCR=0x182E078
GCC_ANOC_PCIE0_1LANE_M_CBCR=0x182E07C
GCC_ANOC_PCIE2_2LANE_M_CBCR=0x182E080
GCC_ANOC_USB_AXI_CBCR=0x182E084
GCC_ANOC1_AXI_CBCR=0x182E088
GCC_ANOC_PCIE1_1LANE_M_CBCR=0x182E08C
GCC_ANOC_PCIE3_2LANE_M_CBCR=0x182E090
GCC_ANOC_PCNOC_AHB_CBCR=0x182E094
GCC_ANOC_QOSGEN_EXTREF_CBCR=0x182E098
GCC_ANOC_XO_DCD_CBCR=0x182E09C
GCC_SNOC_XO_DBG_CBCR=0x182E0A0
GCC_ANOC_XO_DBG_CBCR=0x182E0A4
GCC_ANOC_WCSS_AXI_M_CBCR=0x182E0A8
GCC_USB0_EUD_AT_CBCR=0x1830004
GCC_PCNOC_AHB_CBCR=0x1831018
GCC_PCNOC_XO_DBG_CBCR=0x183101C
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
GCC_QPIC_IO_MACRO_CBCR=0x183200C
GCC_QPIC_AHB_CBCR=0x1832010
GCC_QPIC_CBCR=0x1832014
GCC_QPIC_SLEEP_CBCR=0x1832018
GCC_SDCC1_APPS_CBCR=0x183302C
GCC_SDCC1_ICE_CORE_CBCR=0x1833030
GCC_SDCC1_AHB_CBCR=0x1833034
GCC_XO_CBCR=0x1834018
GCC_XO_DIV4_CBCR=0x183401C
GCC_IM_SLEEP_CBCR=0x1834020
GCC_AHB_CBCR=0x1834024
GCC_DCC_CBCR=0x1835004
GCC_DCC_XO_CBCR=0x1835008
GCC_SPDM_PCNOC_CY_CBCR=0x1836008
GCC_SPDM_RPM_CY_CBCR=0x183600C
GCC_SPDM_SNOC_CY_CBCR=0x1836010
GCC_SPDM_DEBUG_CY_CBCR=0x1836014
GCC_SPDM_FF_CBCR=0x1836018
GCC_MPM_AHB_CBCR=0x1837004
GCC_APC0_VOLTAGE_DROOP_DETECTOR_GPLL0_CBCR=0x183800C
GCC_RBCPR_CBCR=0x183900C
GCC_RBCPR_AHB_CBCR=0x1839010
GCC_RBCPR_MX_CBCR=0x1839020
GCC_RBCPR_MX_AHB_CBCR=0x1839024
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
while [ $var1 -le $Bit25 ]
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
while [ $var2 -le $Bit25 ]
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

#Apply multiplier
var2=`echo - | awk '{print "'"$var2"'" *  "'"$multiplier"'" }'`

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

calculate_clk gcc_gpll0_out_main_div2_cgc_clk 0
calculate_clk gcc_pcnoc_ts_clk 1
calculate_clk gcc_gpll0_out_main_cgc_clk 2
calculate_clk gcc_gpll2_out_main_cgc_clk 3
calculate_clk gcc_gpll4_out_main_cgc_clk 4
calculate_clk gcc_gpll6_out_main_cgc_clk 5
calculate_clk gcc_apss_gpll0_clk_src_clk 6
calculate_clk gcc_apss_gpll2_clk_src_clk 7
calculate_clk gcc_apss_gpll4_clk_src_clk 8
calculate_clk gcc_tme_debug_clk 9
calculate_clk gcc_apss_ahb_clk 10
calculate_clk gcc_apss_axi_clk 11
calculate_clk gcc_q6_axim_clk 12
calculate_clk gcc_q6_axim2_clk 13
calculate_clk gcc_q6_ahb_clk 14
calculate_clk gcc_q6_ahb_s_clk 15
calculate_clk gcc_q6ss_atbm_clk 16
calculate_clk gcc_q6_tsctr_1to2_clk 17
calculate_clk gcc_q6ss_pclkdbg_clk 18
calculate_clk gcc_wcss_dbg_ifc_apb_clk 19
calculate_clk gcc_wcss_dbg_ifc_atb_clk 20
calculate_clk gcc_wcss_dbg_ifc_nts_clk 21
calculate_clk gcc_wcss_dbg_ifc_dapbus_clk 22
calculate_clk gcc_wcss_dbg_ifc_apb_bdg_clk 23
calculate_clk gcc_wcss_dbg_ifc_atb_bdg_clk 24
calculate_clk gcc_wcss_dbg_ifc_nts_bdg_clk 25
calculate_clk gcc_wcss_dbg_ifc_dapbus_bdg_clk 26
calculate_clk gcc_wcss_ecahb_clk 27
calculate_clk gcc_wcss_acmt_clk 28
calculate_clk gcc_wcss_ahb_s_clk 29
calculate_clk gcc_wcss_axi_m_clk 30
calculate_clk gcc_snoc_ts_clk 31
calculate_clk gcc_ubi32_dbg0_clk 32
calculate_clk gcc_nssnoc_atb_clk 33
calculate_clk gcc_nss_ts_clk 34
calculate_clk gcc_nssnoc_qosgen_ref_clk 35
calculate_clk gcc_nssnoc_timeout_ref_clk 36
calculate_clk gcc_nssnoc_memnoc_clk 37
calculate_clk gcc_nssnoc_snoc_clk 38
calculate_clk gcc_nsscfg_clk 39
calculate_clk gcc_lpass_sway_clk 40
calculate_clk gcc_lpass_core_axim_clk 41
calculate_clk gcc_pcie0_ahb_clk 42
calculate_clk gcc_pcie0_aux_clk 43
calculate_clk gcc_pcie0_axi_m_clk 44
calculate_clk gcc_pcie0_axi_s_clk 45
calculate_clk gcc_pcie0_axi_s_bridge_clk 46
calculate_clk gcc_pcie0_pipe_clk 47
calculate_clk gcc_pcie1_ahb_clk 48
calculate_clk gcc_pcie1_aux_clk 49
calculate_clk gcc_pcie1_axi_m_clk 50
calculate_clk gcc_pcie1_axi_s_clk 51
calculate_clk gcc_pcie1_axi_s_bridge_clk 52
calculate_clk gcc_pcie1_pipe_clk 53
calculate_clk gcc_pcie2_ahb_clk 54
calculate_clk gcc_pcie2_aux_clk 55
calculate_clk gcc_pcie2_axi_m_clk 56
calculate_clk gcc_pcie2_axi_s_clk 57
calculate_clk gcc_pcie2_axi_s_bridge_clk 58
calculate_clk gcc_pcie2_pipe_clk 59
calculate_clk gcc_pcie3_ahb_clk 60
calculate_clk gcc_pcie3_aux_clk 61
calculate_clk gcc_pcie3_axi_m_clk 62
calculate_clk gcc_pcie3_axi_s_clk 63
calculate_clk gcc_pcie3_axi_s_bridge_clk 64
calculate_clk gcc_pcie3_pipe_clk 65
calculate_clk gcc_usb0_master_clk 66
calculate_clk gcc_usb0_aux_clk 67
calculate_clk gcc_usb0_mock_utmi_clk 68
calculate_clk gcc_usb0_eud_at_clk 69
calculate_clk gcc_usb0_pipe_clk 70
calculate_clk gcc_usb0_sleep_clk 71
calculate_clk gcc_usb0_phy_cfg_ahb_clk 72
calculate_clk gcc_qdss_at_clk 73
calculate_clk gcc_qdss_stm_clk 74
calculate_clk gcc_qdss_traceclkin_clk 75
calculate_clk gcc_qdss_tsctr_div2_clk 76
calculate_clk gcc_qdss_tsctr_div3_clk 77
calculate_clk gcc_qdss_tsctr_div4_clk 78
calculate_clk gcc_qdss_tsctr_div8_clk 79
calculate_clk gcc_qdss_tsctr_div16_clk 80
calculate_clk gcc_qdss_dap_clk 81
calculate_clk gcc_qdss_apb2jtag_clk 82
calculate_clk gcc_qdss_etr_usb_clk 83
calculate_clk gcc_qdss_dap_ahb_clk 84
calculate_clk gcc_qdss_cfg_ahb_clk 85
calculate_clk gcc_qdss_eud_at_clk 86
calculate_clk gcc_sys_noc_axi_clk 87
calculate_clk gcc_snoc_qosgen_extref_clk 88
calculate_clk gcc_mem_noc_ts_clk 89
calculate_clk gcc_snoc_lpass_cfg_clk 90
calculate_clk gcc_sys_noc_apss_ahb_clk 91
calculate_clk gcc_sys_noc_wcss_ahb_clk 92
calculate_clk gcc_sys_noc_qdss_stm_axi_clk 93
calculate_clk gcc_sys_noc_at_clk 94
calculate_clk gcc_snoc_pcnoc_ahb_clk 95
calculate_clk gcc_snoc_tme_cfg_clk 96
calculate_clk gcc_nssnoc_snoc_1_clk 97
calculate_clk gcc_snoc_pcie0_1lane_s_clk 98
calculate_clk gcc_snoc_pcie1_1lane_s_clk 99
calculate_clk gcc_snoc_pcie2_2lane_s_clk 100
calculate_clk gcc_snoc_pcie3_2lane_s_clk 101
calculate_clk gcc_snoc_usb_clk 102
calculate_clk gcc_snoc_tme_clk 103
calculate_clk gcc_snoc_xo_dcd_clk 104
calculate_clk gcc_anoc_wcss_axi_m_clk105
calculate_clk gcc_anoc0_axi_clk 106
calculate_clk gcc_anoc_pcie0_1lane_m_clk 107
calculate_clk gcc_anoc_pcie2_2lane_m_clk 108
calculate_clk gcc_anoc_usb_axi_clk 109
calculate_clk gcc_anoc1_axi_clk 110
calculate_clk gcc_anoc_pcie1_1lane_m_clk 111
calculate_clk gcc_anoc_pcie3_2lane_m_clk 112
calculate_clk gcc_anoc_pcnoc_ahb_clk 113
calculate_clk gcc_anoc_qosgen_extref_clk 114
calculate_clk gcc_anoc_xo_dcd_clk 115
calculate_clk gcc_pcnoc_ahb_clk 116
calculate_clk gcc_pcnoc_tic_clk 117
calculate_clk gcc_pcnoc_lpass_clk 118
calculate_clk gcc_pcnoc_at_clk 119
calculate_clk gcc_pcnoc_xo_dcd_clk 120
calculate_clk gcc_apss_tcu_async_clk 121
calculate_clk gcc_smmu_cfg_clk 122
calculate_clk gcc_pcnoc_tbu_clk 123
calculate_clk gcc_anoc0_tbu_clk 124
calculate_clk gcc_nss_tbu_clk125
calculate_clk gcc_anoc1_tbu_clk 126
calculate_clk gcc_wcss_q6_tbu_clk 127
calculate_clk gcc_tme_clk 128
calculate_clk gcc_nssnoc_pcnoc_1_clk 129
calculate_clk gcc_nssnoc_mem_noc_1_clk 130
calculate_clk gcc_tme_ts_clk 131
calculate_clk gcc_q6ss_trig_clk 132
calculate_clk gcc_qdss_ts_clk 133
calculate_clk gcc_ddrss_sms_slow_clk 134
calculate_clk gcc_tme_slow_clk 135
calculate_clk gcc_tme_dmi_dbg_hs_clk 136
calculate_clk gcc_tme_rtc_toggle_clk 137
calculate_clk gcc_rpm_proc_fclk 138
calculate_clk gcc_rpm_proc_hclk 139
calculate_clk gcc_rpm_bus_ahb_clk 140
calculate_clk gcc_rpm_sleep_clk 141
calculate_clk gcc_rpm_timer_clk 142
calculate_clk gcc_rpm_mst_m2_noc_ahb_clk 143
calculate_clk gcc_rpm_noc_mst_ahb_clk 144
calculate_clk gcc_qpic_io_macro_clk 145
calculate_clk gcc_qpic_ahb_clk 146
calculate_clk gcc_qpic_clk 147
calculate_clk gcc_qpic_sleep_clk 148
calculate_clk gcc_blsp1_ahb_clk 149
calculate_clk gcc_blsp1_sleep_clk 150
calculate_clk gcc_blsp1_qup1_spi_apps_clk 151
calculate_clk gcc_blsp1_qup1_i2c_apps_clk 152
calculate_clk gcc_blsp1_uart1_apps_clk 153
calculate_clk gcc_blsp1_qup2_spi_apps_clk 154
calculate_clk gcc_blsp1_qup2_i2c_apps_clk 155
calculate_clk gcc_blsp1_uart2_apps_clk 156
calculate_clk gcc_blsp1_qup3_spi_apps_clk 157
calculate_clk gcc_blsp1_qup3_i2c_apps_clk 158
calculate_clk gcc_blsp1_uart3_apps_clk 159
calculate_clk gcc_blsp1_qup4_spi_apps_clk 160
calculate_clk gcc_blsp1_qup4_i2c_apps_clk 161
calculate_clk gcc_blsp1_uart4_apps_clk 162
calculate_clk gcc_blsp1_qup5_spi_apps_clk 163
calculate_clk gcc_blsp1_qup5_i2c_apps_clk 164
calculate_clk gcc_blsp1_uart5_apps_clk 165
calculate_clk gcc_blsp1_qup6_spi_apps_clk 166
calculate_clk gcc_blsp1_qup6_i2c_apps_clk 167
calculate_clk gcc_blsp1_uart6_apps_clk 168
calculate_clk gcc_sdcc1_apps_clk 169
calculate_clk gcc_sdcc1_ice_core_clk 170
calculate_clk gcc_sdcc1_ahb_clk 171
calculate_clk gcc_xo_clk 172
calculate_clk gcc_xo_div4_clk 173
calculate_clk gcc_im_sleep_clk 174
calculate_clk gcc_ahb_clk 175
calculate_clk gcc_sec_ctrl_acc_clk 176
calculate_clk gcc_sec_ctrl_clk 177
calculate_clk gcc_sec_ctrl_sense_clk 178
calculate_clk gcc_sec_ctrl_ahb_clk 179
calculate_clk gcc_sec_ctrl_boot_rom_patch_clk 180
calculate_clk gcc_mdio_ahb_clk 181
calculate_clk gcc_tcsr_ahb_clk 182
calculate_clk gcc_ddrss_atb_clk 183
calculate_clk gcc_ddrss_ahb_clk 184
calculate_clk gcc_mem_noc_ahb_clk 185
calculate_clk gcc_mem_noc_q6_axi_clk 186
calculate_clk gcc_mem_noc_nssnoc_clk 187
calculate_clk gcc_mem_noc_snoc_axi_clk 188
calculate_clk gcc_mem_noc_apss_axi_clk 189
calculate_clk gcc_mem_noc_apss_tcu_clk 190
calculate_clk gcc_mem_noc_qosgen_extref_clk 191
calculate_clk gcc_adss_pwm_clk 192
calculate_clk gcc_tlmm_ahb_clk 193
calculate_clk gcc_tlmm_clk 194
calculate_clk gcc_msg_ram_ahb_clk 195
calculate_clk gcc_tic_clk 196
calculate_clk gcc_spdm_pcnoc_cy_clk 197
calculate_clk gcc_spdm_rpm_cy_clk 198
calculate_clk gcc_spdm_snoc_cy_clk 199
calculate_clk gcc_spdm_debug_cy_clk 200
calculate_clk gcc_spdm_ff_clk 201
calculate_clk gcc_prng_ahb_clk 202
calculate_clk gcc_boot_rom_ahb_clk 203
calculate_clk gcc_imem_axi_clk 204
calculate_clk gcc_imem_cfg_ahb_clk 205
calculate_clk gcc_uniphy0_sys_clk 206
calculate_clk gcc_uniphy0_ahb_clk 207
calculate_clk gcc_uniphy1_sys_clk 208
calculate_clk gcc_uniphy1_ahb_clk 209
calculate_clk gcc_uniphy2_sys_clk 210
calculate_clk gcc_uniphy2_ahb_clk 211
calculate_clk gcc_cmn_12gpll_ahb_clk 212
calculate_clk gcc_cmn_12gpll_sys_clk 213
calculate_clk gcc_cmn_12gpll_apu_clk 214
calculate_clk gcc_mpm_ahb_clk 215
calculate_clk gcc_crypto_clk 216
calculate_clk gcc_crypto_axi_clk 217
calculate_clk gcc_crypto_ahb_clk 218
calculate_clk gcc_rbcpr_clk 219
calculate_clk gcc_rbcpr_ahb_clk 220
calculate_clk gcc_rbcpr_mx_clk 221
calculate_clk gcc_rbcpr_mx_ahb_clk 222
calculate_clk gcc_snoc_xo_dbg_clk 223
calculate_clk gcc_anoc_xo_dbg_clk 224
calculate_clk gcc_pcnoc_xo_dbg_clk 225
calculate_clk gcc_pcnoc_bus_timeout0_ahb_clk 226
calculate_clk gcc_pcnoc_bus_timeout1_ahb_clk 227
calculate_clk gcc_pcnoc_bus_timeout2_ahb_clk 228
calculate_clk gcc_pcnoc_bus_timeout3_ahb_clk 229
calculate_clk gcc_pcnoc_bus_timeout4_ahb_clk 230
calculate_clk gcc_pcnoc_bus_timeout5_ahb_clk 231
calculate_clk gcc_pcnoc_bus_timeout6_ahb_clk 232
calculate_clk gcc_pcnoc_bus_timeout7_ahb_clk 233
calculate_clk gcc_pcnoc_bus_timeout8_ahb_clk 234
calculate_clk gcc_pcnoc_bus_timeout9_ahb_clk 235
calculate_clk gcc_nssnoc_xo_dcd_clk 236
calculate_clk gcc_gp1_clk 237
calculate_clk gcc_gp2_clk 238
calculate_clk gcc_gp3_clk 239
calculate_clk gcc_dcc_clk 240
calculate_clk gcc_apc0_voltage_droop_detector_gpll0_clk 241
calculate_clk usb30_utmi_clk 242
calculate_clk usb0_hspw_clk_480m_test 243
calculate_clk apss_gcc_dbg_clk 244
calculate_clk mpm_gcc_temp_sensor_ringosc_clk 245
calculate_clk pciephy0_debug_clk 246
calculate_clk pciephy1_debug_clk 247
calculate_clk pciephy2_debug_clk 248
calculate_clk pciephy3_debug_clk 249
calculate_clk ddr_pll_dtest 250
calculate_clk lpass_dbg_clk 251
calculate_clk q6ss_pll_dtest 252
calculate_clk q6ss_dbg_clk 253
calculate_clk wcss_gcc_debug_clk_obs 254
calculate_clk wcss_zinc_dbg_out_clk 255
calculate_clk ubi32_pll_dtest 256
calculate_clk nss_cc_debug_clk 257
calculate_clk gcc_nssnoc_nsscc_clk 258
calculate_clk gcc_nsscc_clk 259
calculate_clk gcc_dcc_xo_clk 260
calculate_clk q6ss_core_clk_div8_mon 261
calculate_clk gcc_tme_atb_clk 262
calculate_clk gcc_tme_dbgapb_clk 263
calculate_clk gcc_mem_noc_xo_dbg_clk 264
calculate_clk gcc_tme_at_clk 265
calculate_clk gcc_tme_apb_clk 266
calculate_clk gcc_tme_bus_clk 267
calculate_clk gpll0_dtest 268
calculate_clk gpll2_dtest 269
calculate_clk gpll4_dtest 270
calculate_clk gpll6_dtest 271
calculate_clk gcc_q6ss_boot_clk 272
calculate_clk bias_pll_ubi_nc_clk 273
calculate_clk gcc_pcie0_rchng_clk 274
calculate_clk gcc_pcie1_rchng_clk 275
calculate_clk gcc_pcie2_rchng_clk 276
calculate_clk gcc_pcie3_rchng_clk 277
