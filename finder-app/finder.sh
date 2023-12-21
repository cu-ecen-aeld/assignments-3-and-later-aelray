#!/bin/sh

if [ "$#" -ne 2 ]; then
    echo "illegal number of parameters"
    exit 1
fi

if ! [ -d "$1" ]; then
    echo "directory doesn't exist"
    exit 1
fi

number_of_files=`find $1 -type f | wc -l`

number_of_matches=`grep -R $2 $1 | wc -l`

echo "The number of files are ${number_of_files} and the number of matching lines are ${number_of_matches}"