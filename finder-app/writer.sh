#!/bin/sh
# 
# Author: Nileshkartik Ashokkumar

set -e
set -u

WRITE_FILE=0
WRITE_STR=0
if [ $# -gt 0 ] 
then
	if [ $# -lt 3 ]
	then
		if [ $# -lt 2 ]
		then 
			echo "Argument_1 is missing"
			exit 1
		else
			
			if [ -d "$(dirname "$1")" ]
			then
				if [ -f "$(basename "$1")" ]
				then 
					echo "directory exist, file exist" 
					echo "$2" > "$1"
				else
					echo "directory exist, creating file"
					touch "$(basename "$1")"
					echo "$2" > "$1"
				fi
			else
				echo "creating directory"
				mkdir -p "$(dirname "$1")" 
				cd "$(dirname "$1")"
				touch "$1" 
				echo "$2" > "$1"
			fi
		fi
	else
		echo "greater than 2 Arguments"
		exit 1
		fi
else
	echo "Argument_0 and Argument_1 is missing"
	exit 1
	fi

