#!/bin/sh
: '
  *****************************************************************************/
​ * Copyright​ ​(C)​ ​2022​ ​by​ ​Nileshkartik Ashokkumar
​ *
​ * ​​Redistribution,​ ​modification​ ​or​ ​use​ ​of​ ​this​ ​software​ ​in​ ​source​ ​or​ ​binary
​ * ​​forms​ ​is​ ​permitted​ ​as​ ​long​ ​as​ ​the​ ​files​ ​maintain​ ​this​ ​copyright.​ ​Users​ ​are
​ ​* ​permitted​ ​to​ ​modify​ ​this​ ​and​ ​use​ ​it​ ​to​ ​learn​ ​about​ ​the​ ​field​ ​of​ ​embedded
​ * software.​ Nileshkartik Ashokkumar ​and​ ​the​ ​University​ ​of​ ​Colorado​ ​are​ ​not​ ​liable​ ​for
​ ​* ​any​ ​misuse​ ​of​ ​this​ ​material.
​ * 
 *****************************************************************************/
​ ​*​ ​@file​ ​writer.sh
​ ​*​ ​@brief​ ​a shell script that takes 2 arguments from the user, 1 st argument is file path and 2nd argument is a word that has to be overwritten in file mentioned in 1 st argument 
​ ​*
​ ​*
​ ​*​ ​@author​ ​Nileshkartik Ashokkumar
​ ​*​ ​@date​ ​Jan​ ​23​ ​2023
​ ​*​ ​@version​ ​1.0
​ ​*
​ ​'


WRITE_FILE=$1								#store the argument 1 in a variable
WRITE_STR=$2								#store the argument 2 in a variable

if [ $# -ne 2 ] 
then
	echo "Invalid arguments"					#check if the arguments are not equal to 2, print invalid and exit 1
	exit 1
fi

if [ -d "$(dirname "$WRITE_FILE")" ]				#check if the directory is valid, where dirname strips the path except file name
then
	if [ -f "$(basename "$WRITE_FILE")" ]			#check if the file is valid, where basename extracts only the last word from the directory 
	then 
		echo "$WRITE_STR" > "$WRITE_FILE"		#overwrite the word from 2nd argument to file mentioned in the 1st argument 
	else
		touch "$(basename "$WRITE_FILE")"		#create the file if the file doesnt exists
		echo "$WRITE_STR" > "$WRITE_FILE"		#overwrite the word from 2nd argument to file mentioned in the 1st argument 
	fi
else
	mkdir -p "$(dirname "$WRITE_FILE")" 			#create directory if the directory doesnt exists
	cd "$(dirname "$WRITE_FILE")"				#change directory to the created directory
	touch "$WRITE_FILE" 					#create the file mentioned by the user in the 1st argument
	echo "$WRITE_STR" > "$WRITE_FILE"			#overwrite the word from 2nd argument to file mentioned in the 1st argument 
fi


