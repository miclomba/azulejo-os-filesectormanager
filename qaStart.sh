#! /bin/bash
#/*********************************
# *	File Sector Manager (FSM)
# *	Author: Michael Lombardi
# **********************************/

rm aMap fMap iMap hardDisk qaOutput.txt
touch aMap fMap iMap hardDisk qaOutput.txt

make clean
make

rm main.o
#Use input file
./fsm 1 > qaOutput.txt

#Use stub input file
#./fsm 1 1 > qaOutput.txt

