#!/bin/sh

if [ "$#" -ne 2 ]; then
    echo "illegal number of parameters"
    exit 1
fi

dir_name=`dirname $1`

if ! [ -d "$dir_name" ]; then
    mkdir -p $dir_name
fi

echo $2 > $1