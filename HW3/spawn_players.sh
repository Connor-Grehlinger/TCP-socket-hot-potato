#!/bin/bash
for (( i = 0; i < 10; i++ ))
do
    ./player ::1 3333 $i $[2*$i]
done
