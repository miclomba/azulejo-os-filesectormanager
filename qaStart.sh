#! /bin/bash
#/*********************************
# *	File Sector Manager (FSM)
# *	Author: Michael Lombardi
# **********************************/

make touch_data
make clean
make

#Use input file
./test/fsm 1 > test/qaOutput.txt

#Use stub input file
#./test/fsm 1 1 > test/qaOutput.txt

