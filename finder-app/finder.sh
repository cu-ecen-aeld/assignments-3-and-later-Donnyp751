#!/bin/bash
# finder.sh filesdir searchstr
# where
# filesdir is the base directory to search for a file with the searchstr content
# searchstr is the string to search for inside files contained in the filesdir


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
