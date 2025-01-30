#!/bin/bash
#

filesdir=$1
searchstr=$2

if [[ $# -ne 2 ]]; then
	echo "Invalid args specified"
	exit 1
fi

if [[ ! -d $filesdir ]]; then
	echo "Directory $filesdir does not exist"
	exit 1
fi

mLineCount=$(grep -r $searchstr $filesdir | wc -l)
mFileCount=$(find $filesdir -type f -name "*" -exec grep -l "$searchstr" {} + | wc -l)

echo "The number of files are $mFileCount and the number of matching lines are $mLineCount"
