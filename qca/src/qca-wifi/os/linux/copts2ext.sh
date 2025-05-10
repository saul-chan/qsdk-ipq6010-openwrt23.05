#
# Copyright (c) 2023 Qualcomm Innovation Center, Inc.
#
# All Rights Reserved.
# Confidential and Proprietary - Qualcomm Innovation Center, Inc.
#
#

rm $1.nl
cat $1 | tr ' ' '\n' > $1.nl

while read line
do
	if [[ $(echo $line | grep '^\-D') ]]; then
#remove preceding empty spaces, followed by -D, and =
		printf "#ifndef %s\n" `echo $line|cut -f 1 -d '=' | sed -e 's/^\s*//g' | sed -e 's/-D//g'` >>$2
		printf "#define %s\n" `echo $line|sed -e 's/^-D//g'` | sed -e 's/=/ /' >> $2
		printf "#endif\n" >> $2
		echo  >> $2
	else
		if [[ $(echo $line | grep '^\-U') ]]; then
			printf "#undef %s\n" `echo $line | sed -e 's/-U//g'` >>$2
			echo  >> $2
		else
			echo "Neither -D nor -U found" $line  1>&2
		fi
	fi
done < $1.nl
