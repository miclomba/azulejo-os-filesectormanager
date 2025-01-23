#! /bin/bash
#/*********************************
# *	File Sector Manager (FSM)
# *	Author: Michael Lombardi
# **********************************/

rm aMap fMap iMap hardDisk qaOutput.txt
touch aMap fMap iMap hardDisk qaOutput.txt

make clean
make

rm fsm.o
#Use program output
./fsm 1 > qaOutput.txt

#Use perfect output
#./fsm 1 1 > qaOutput.txt

