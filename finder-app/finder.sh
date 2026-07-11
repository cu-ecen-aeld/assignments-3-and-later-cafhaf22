#!/bin/bash
if [ $# -eq 0 ]
then
    echo "ERROR: No directory path and search string were provided."
    echo "Please provide a directory path <filesdir> and a search string <searchstr>."
    exit 1
elif [ $# -eq 1 ]
then
    echo "ERROR: Search string not provided."
    echo "Please provide a search string <searchstr>."
    exit 1
fi

filesdir=$1
searchstr=$2

if [ ! -d $filesdir ]
then
    echo "ERROR: $filesdir does not exists!."
    exit 1
fi

numFiles=$( ls $filesdir | wc -l )
numMatches=$( grep -r $searchstr $filesdir -o | wc -l)
echo "The number of files are $numFiles and the number of matching lines are $numMatches"