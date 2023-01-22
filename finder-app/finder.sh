#!/bin/sh
# 
# Author: Nileshkartik Ashokkumar

set -e
set -u

NO_FILES=0
NO_COUNT=0
if [ $# -gt 0 ] 
then
	if [ $# -lt 3 ]
	then
		if [ $# -lt 2 ]
		then 
			echo "only one arg"
			exit 1
		else
			
			if [ -d "$1" ]
			then
				NO_FILES="$(find "$1" -type f | wc -l)"
				NO_COUNT="$(grep -r "$2" "$1"| wc -l)"
				#NO_FILES=$(find $1 -type f | wc -l)
				echo "The number of files are ${NO_FILES} and the number of matching lines are ${NO_COUNT} " 
			else
				echo "$1 not found"
				exit 1
			fi
		fi
	else
		echo "greater than 2 args"
		exit 1
		fi
else
	echo "less arguments"
	exit 1
	fi

