#!/bin/bash

export LANG=C

if [ $# -lt 1 ]; then
  echo Usage: urlencode string
  exit 1
fi

arg="$1"
i="0"
while [ "$i" -lt ${#arg} ]; do
    c=${arg:$i:1}
    if [ "$c" = "&" ]; then
        printf "%%26"
    else
        printf "$c"
    fi
    i=$((i+1))
done

