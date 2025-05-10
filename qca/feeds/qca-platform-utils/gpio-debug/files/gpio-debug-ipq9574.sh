: '
/*
 * Copyright (c) 2019,2021 Qualcomm Technologies, Inc.
 *
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */
'

#!/bin/ash
IO_MASK=$(( 0x00000200 ))
BIAS_STATUS_MASK=$(( 0x00000003 ))
FUNC_SEL_MASK=$(( 0x0000003c ))
DRV_STRENGTH_MASK=$(( 0x000001c0 ))
TLMM_GPIO_CFGn_Base_Address=$(( 0x01000000 ))
OFFSET_MULTIPLIER=$(( 0x00001000 ))
START_PIN=0
END_PIN=64
FUNCTION_SELECT_MAPPING=$(cat <<-END
0, sdc7, qspi3, qdssNAtraceclkNAb, NA, NA, NA, NA, NA, NA
1, sdc6, qspi2, qdssNAtracectlNAb, NA, NA, NA, NA, NA, NA
2, sdc5, qspi1, qdssNAtracedataNAb, NA, NA, NA, NA, NA, NA
3, sdc4, qspi0, qdssNAtracedataNAb, NA, NA, NA, NA, NA, NA
4, sdcNAcmd, qspiNAcs, qdssNAtracedataNAb, NA, NA, NA, NA, NA, NA
5, sdcNAclk, qspiNAclk, qdssNAtracedataNAb, NA, NA, NA, NA, NA, NA
6, sdc3, qdssNAtracedataNAb, NA, NA, NA, NA, NA, NA, NA
7, sdc2, qdssNAtracedataNAb, NA, NA, NA, NA, NA, NA, NA
8, sdc1, qdssNAtracedataNAb, NA, NA, NA, NA, NA, NA, NA
9, sdc0, qdssNAtracedataNAb, NA, NA, NA, NA, NA, NA, NA
10, sdcNArclk, qdssNAtracedataNAb, NA, NA, NA, NA, NA, NA, NA
11, blsp0NAspi, blsp0NAuart, qdssNAtracedataNAb, NA, NA, NA, NA, NA, NA
12, blsp0NAspi, blsp0NAuart, qdssNAtracedataNAb, NA, NA, NA, NA, NA, NA
13, blsp0NAspi, blsp0NAuart, qdssNAtracedataNAb, NA, NA, NA, NA, NA, NA
14, blsp0NAspi, blsp0NAuart, qdssNAtracedataNAb, NA, NA, NA, NA, NA, NA
15, blsp3NAspi, blsp3NAi2c, blsp3NAuart, qdssNAtracedataNAb, NA, NA, NA, NA, NA
16, blsp3NAspi, blsp3NAi2c, blsp3NAuart, qdssNAtracedataNAb, NA, NA, NA, NA, NA
17, blsp3NAspi, blsp3NAuart, dbgNAout, qdssNAtracedataNAb, NA, NA, NA, NA, NA
18, blsp3NAspi, blsp3NAuart, NA, NA, NA, NA, NA, NA, NA
19, blsp3NAspi, NA, NA, NA, NA, NA, NA, NA, NA
20, blsp3NAspi, NA, criNAtrng0, NA, NA, NA, NA, NA, NA
21, blsp3NAspi, NA, criNAtrng1, NA, NA, NA, NA, NA, NA
22, pcie0NAclk, NA, pta20, NA, NA, NA, NA, NA, NA
23, NA, pta20, wci21, cxc0, NA, NA, NA, NA, NA
24, pcie0NAwake, NA, pta20, wci21, cxc0, NA, qdssNActiNAtrigNAoutNAb0, NA, NA
25, pcie1NAclk, NA, NA, qdssNActiNAtrigNAinNAb0, NA, NA, NA, NA, NA
26, NA, atestNAchar0, NA, qdssNActiNAtrigNAoutNAb1, NA, NA, NA, NA, NA
27, pcie1NAwake, NA, atestNAchar1, qdssNActiNAtrigNAinNAb1, NA, NA, NA, NA, NA
28, pcie2NAclk, atestNAchar2, NA, NA, NA, NA, NA, NA, NA
29, atestNAchar3, NA, NA, NA, NA, NA, NA, NA, NA
30, pcie2NAwake, pwm01, atestNAchar, NA, NA, NA, NA, NA, NA
31, pcie3NAclk, pwm11, NA, qdssNActiNAtrigNAinNAa1, NA, NA, NA, NA, NA
32, pwm21, NA, qdssNActiNAtrigNAoutNAa1, NA, NA, NA, NA, NA, NA
33, pcie3NAwake, pwm31, NA, qdssNActiNAtrigNAinNAa0, NA, NA, NA, NA, NA
34, blsp2NAuart, blsp2NAi2c, blsp2NAspi, blsp1NAuart, NA, criNAtrng1, qdssNActiNAtrigNAoutNAa0, NA, NA
35, blsp2NAuart, blsp2NAi2c, blsp2NAspi, blsp1NAuart, NA, criNAtrng2, NA, NA, NA
36, blsp1NAuart, blsp1NAi2c, blsp2NAspi, NA, criNAtrng3, NA, NA, NA, NA
37, blsp1NAuart, blsp1NAi2c, blsp2NAspi, NA, dwcNAddrphy, NA, NA, NA, NA
38, mdc, NA, criNAtrng0, NA, NA, NA, NA, NA, NA 
39, mdio, NA, NA, NA, NA, NA, NA, NA, NA
40, audioNApri, audioNApdm0, NA, NA, NA, NA, NA, NA, NA
41, audioNApri, audioNApdm0, NA, NA, NA, NA, NA, NA, NA
42, audioNApri, audioNApdm0, NA, NA, NA, NA, NA, NA, NA
43, audioNApri, audioNApdm0, NA, qdssNAtraceclkNAa, NA, NA, NA, NA, NA
44, pwm00, audioNAsec, wsaNAswrm, NA, qdssNAtracectlNAa, NA, NA, NA, NA
45, pwm10, audioNAsec, wsaNAswrm, NA, qdssNAtracedataNAa, NA, NA, NA, NA
46, pwm20, audioNAsec, rx1, mac00, NA, qdssNAtracedataNAa, NA, NA, NA
47, pwm30, audioNAsec, mac01, NA, qdssNAtracedataNAa, NA, NA, NA, NA
48, blsp5NAi2c, blsp5NAuart, NA, qdssNAtracedataNAa, NA, NA, NA, NA, NA
49, blsp5NAi2c, blsp5NAuart, NA, qdssNAtracedataNAa, NA, NA, NA, NA, NA
50, blsp4NAuart, blsp4NAi2c, blsp4NAspi, pwm02, qdssNAtracedataNAa, NA, NA, NA, NA
51, blsp4NAuart, blsp4NAi2c, blsp4NAspi, pwm12, qdssNAtracedataNAa, NA, NA, NA, NA
52, blsp4NAuart, blsp4NAspi, pwm22, qdssNAtracedataNAa, NA, NA, NA, NA, NA
53, blsp4NAuart, blsp4NAspi, pwm32, qdssNAtracedataNAa, NA, NA, NA, NA, NA
54, pta1NA1, pwm04, qdssNAtracedataNAa, NA, NA, NA, NA, NA, NA
55, pta1NA2, pwm14, qdssNAtracedataNAa, NA, NA, NA, NA, NA, NA
56, pta1NA0, pwm24, qdssNAtracedataNAa, NA, NA, NA, NA, NA, NA
57, wci20, cxc1, mac10, pwm03, qdssNAtracedataNAa, NA, NA, NA, NA
58, wci20, cxc1, mac11, pwm13, qdssNAtracedataNAa, NA, NA, NA, NA
59, rx0, pwm23, qdssNAtracedataNAa, NA, NA, NA, NA, NA, NA
60, pwm33, prngNArosc0, qdssNAtracedataNAa, NA, gccNAplltest, NA, NA, NA, NA
61, blsp1NAspi, audioNApri, audioNApdm1, audioNApri, pta21, prngNArosc1, gccNAtlmm, NA, NA
62, blsp1NAspi, audioNAsec, audioNApdm1, audioNAsec, pta21, prngNArosc2, gccNAplltest, NA, NA
63, blsp1NAspi, audioNApdm1, pta21, prngNArosc3, NA, NA, NA, NA, NA
64, blsp1NAspi, audioNApdm1, tsensNAmax, NA, NA, NA, NA, NA, NA
END
)

FUNCTION_SEL()
{
 pin=$1
 pin=$( expr "$pin" + "1" )
 func_col=$2
 func_col=$( expr "$func_col" + "1" )
 FUNCTION_SEL_VALUE=`echo "$FUNCTION_SELECT_MAPPING" | sed -n "$pin"p`
 func_value=`echo "$FUNCTION_SEL_VALUE" | cut -d, -f$func_col`
 printf "\t"
 printf "  $2 -$func_value"
}

GPIO_DUMP()
{
 printf $1
 PIN_OFFSET=$( expr "$OFFSET_MULTIPLIER" \* "$1" )
 STATUS_REGISTER_ADDRESS=$( expr "$TLMM_GPIO_CFGn_Base_Address" + "$PIN_OFFSET" )
 printf "\t\t"
 printf '0x%08x' $STATUS_REGISTER_ADDRESS
 STATUS_MUX=$( devmem $STATUS_REGISTER_ADDRESS )
 printf "\t\t"
 printf $STATUS_MUX
 BIAS_STATUS=$(( $STATUS_MUX & $BIAS_STATUS_MASK ))
 if [ "$BIAS_STATUS" == "0" ]
 then
        printf "\t\tNO PULL  "
 elif [ "$BIAS_STATUS" == "1" ]
 then
        printf "\t\tPULL DOWN"
 elif [ "$BIAS_STATUS" == "2" ]
 then
        printf "\t\tKEEPER   "
 elif [ "$BIAS_STATUS" == "3" ]
 then
        printf "\t\tPULL UP  "
 else
        printf "\tCANNOT BE DETERMINED"
 fi
 FUNC_SEL=$(( $STATUS_MUX & $FUNC_SEL_MASK ))
 FUNC_SEL=$(( $FUNC_SEL >> 2 ))
 if [ "$FUNC_SEL" == "0" ]
 then
        IO_STATUS=$(( $STATUS_MUX & $IO_MASK ))
        IO_STATUS=$(( $IO_STATUS >> 9 ))
        if [ "$IO_STATUS" == "0" ]
        then
                printf "\t\tIP"
        else
                printf "\t\tOP"
        fi
 else
        printf "\t\tNA"
 fi
 DRV_STRENGTH=$(( $STATUS_MUX & $DRV_STRENGTH_MASK ))
 DRV_STRENGTH=$(( $DRV_STRENGTH >> 6 ))
 printf "\t\t$( expr "$DRV_STRENGTH" + "$DRV_STRENGTH" + "2" ) MA"
 if [ "$FUNC_SEL" == "0" ]
 then
	printf " General purpose"
 else
	FUNCTION_SEL "$1" "$FUNC_SEL"
 fi
}

printf "PIN\t    TLMM_GPIO_CFGn_REG_ADD    TLMM_GPIO_CFGn\t      BIAS_STATUS\t     I/P or O/P\t   DRV_STRENGTH\t    FUNC_SEL\n\n"

if [ $1 == "all" ]
then
        for i in `seq $START_PIN $END_PIN`
        do
        GPIO_DUMP "$i"
        printf "\n"
        done
else
        for i in "$@"
        do
        GPIO_DUMP "$i"
        printf "\n"
        done
fi
