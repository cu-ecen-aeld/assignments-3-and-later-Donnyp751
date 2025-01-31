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

#search all files in filesdir for searchstr, then count the number of matches
mLines=$(grep -r $searchstr $filesdir)
mLineCount=$(grep -r $searchstr $filesdir | wc -l)

#search all files in filesdir for searchstr, then count the number of files that contain matches

mFileCount=$(find $filesdir -type f -name "*" -exec grep -l "$searchstr" {} + | uniq | wc -l)



echo "The number of files are $mFileCount and the number of matching lines are $mLineCount"
