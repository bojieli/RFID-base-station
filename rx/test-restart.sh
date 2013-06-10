#!/bin/bash
while [ 0 ]; do 
    ./main $1 100 &
    sleep 1
    pkill main
done
