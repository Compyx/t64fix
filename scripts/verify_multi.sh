#!/bin/bash

# Small test script to see if the exit code of t64fix is correct

find . -type f -iname '*.t64' -print0 | while IFS= read -r -d '' t64; do
    printf 'verifying "%s" .. ' "$t64"
    t64fix -q "$t64"
    if [ "$?" -eq "0" ]
    then
        echo "OK"
    else
        echo "failed"
    fi
done


