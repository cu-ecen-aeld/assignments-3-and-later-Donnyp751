#!/bin/bash
# writer.sh writefile writestr
# where
# writefile is the path/filename to be created and written to
# writestr is the string to be written into the file

writefile=$1
writestr=$2
writedir=$(dirname "$writefile")

#arg count check, requires 2 args
if [[ $# -ne 2 ]]; then
	exit 1
fi


#make the full directory if doesn't already exist
mkdir -p $writedir

#if the directory cannot be created due to error, exit here
if [[ $? -ne 0 ]]; then
	exit 2
fi

#create the file with the string content, overwriting if already existing
echo "$writestr" > $writefile
if [[ $? -ne 0 ]]; then
	echo "Failed to write to file $writefile"
	exit 1
fi
