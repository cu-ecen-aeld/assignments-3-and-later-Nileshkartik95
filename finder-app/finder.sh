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
​ ​*​ ​@file​ ​finder.sh
​ ​*​ ​@brief​ ​a shell script that takes 2 arguments from the user, 1 st argument is file path and 2nd argument is a word
 * The Functionality of this shell script is to count the presence of the 2nd argument i.e.  word in the files mentioned by the user in the 1st argument
​ ​*
​ ​*
​ ​*​ ​@author​ ​Nileshkartik Ashokkumar
​ ​*​ ​@date​ ​Jan​ ​23​ ​2022
​ ​*​ ​@version​ ​1.0
​ ​*
'

NO_FILES=0
NO_COUNT=0
FILE_PATH=$1
FIND_WORD=$2

if [ $# -eq 2 ] 													#check if the no of arguments are 2
then		
	if [ -d "$FILE_PATH" ]												#check if the directory exists
	then
		NO_FILES="$(find "$FILE_PATH" -type f | wc -l)"							#count the total no of files in the given path
		NO_COUNT="$(grep -r "$FIND_WORD" "$FILE_PATH"| wc -l)"						#count the word in all files in the given path
		echo "The number of files are ${NO_FILES} and the number of matching lines are ${NO_COUNT} " 	#print the ouput
	else
		echo "$FILE_PATH not found"										#debug msg to print the file not found if the directory is invlaid
		exit 1
	fi
else
	echo "invalid arguments"											#debug msg if user enters less than 2 arguments
	exit 1
fi

