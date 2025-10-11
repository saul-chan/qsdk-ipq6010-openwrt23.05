#!/bin/sh
ntoa(){                                                                                                 
        echo "$1" | awk '{c=256;print int($0/c^3)"."int($0%c^3/c^2)"."int($0%c^3%c^2/c)"."$0%c^3%c^2%c}'
}                                                                                         
                                                                                          
aton(){                                                                                   
        echo "$1" | awk '{c=256;split($0,ip,".");print ip[4]+ip[3]*c+ip[2]*c^2+ip[1]*c^3}' 
}                                                                                         
                                                      
dtom(){                                               
        local i=$((~0))                               
        local n=$((32-$1))                            
        i=$(($i<<$n))                                 
        echo $i                                       
}                                                     
                                                      
atom(){                                               
        local mask=$(aton $1)                         
        local i=0                                     
        local n=0
         i=31                                         
         while [ $i -gt 0 -o $i -eq 0 ]; do           
                if [[ $((mask&(1<<$i))) -gt 0 ]]; then
                        n=$((n+1))                    
                fi                                    
                i=$((i-1))                            
         done                                         
         echo $(dtom $n)                              
}     

prefixlen(){
        local mask=$(aton $1)
        local i=0
        local n=0
         i=31
         while [ $i -gt 0 -o $i -eq 0 ]; do
                if [[ $((mask&(1<<$i))) -gt 0 ]]; then
                        n=$((n+1))
                fi
                i=$((i-1))
         done
	echo $n
}


getBroadcast(){
	local ip=$1
	local netmask=$2
	local ipn=$(aton $ip)
	local mask=$(atom $netmask)
	local subnet=$((ipn&mask))
	local broadcast=$((~(subnet^mask)))
	broadcast=$(ntoa $broadcast)
	echo "$broadcast"
}

#prefixlen "255.255.255.0"
