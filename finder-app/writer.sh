#!/bin/bash
# writer.sh writefile writestr
# where
# writefile is the path/filename to be created and written to
# writestr is the string to be written into the file

writefile=$1
writestr=$2
writedir=$(dirname "$writefile")


if [[ $# -ne 2 ]]; then
	exit 1
fi


mkdir -p $writedir
if [[ $? -ne 0 ]]; then
	exit 2
fi

echo "$writestr" > $writefile
if [[ $? -ne 0 ]]; then
	echo "Failed to write to file $writefile"
	exit 1
fi
