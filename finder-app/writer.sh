#!/bin/bash
if [ $# -eq 0 ]
then
    echo "ERROR: No file path and write string were provided."
    echo "Please provide a file path <writefile> and a write string <writestr>."
    exit 1
elif [ $# -eq 1 ]
then
    echo "ERROR: Write string not provided."
    echo "Please provide a wrtite string <writestr>."
    exit 1
fi

writefile=$1
writestr=$2
writepath=$( dirname "$writefile" )

mkdir -p "$writepath"

if ! echo "$writestr" > "$writefile";
then
    echo "ERROR: file could not be created."
    exit 1
fi
