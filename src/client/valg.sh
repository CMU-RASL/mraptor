#!/bin/sh
valgrind --num-callers=15 --log-file=/tmp/clawLog.txt --leak-check=yes --suppressions=./valgrind.supp claw

