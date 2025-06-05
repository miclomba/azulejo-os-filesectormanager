#! /bin/bash
#/*********************************
# *	File Sector Manager (FSM)
# *	Author: Michael Lombardi
# **********************************/

echo ">>> Initializing FS Data..."
make touch_data
echo ">>> Cleaning the Project..."
make clean
echo ">>> Building..."
make

#Use input file
echo ">>> Running the Driver..."
./test/fsm 1 > test/qaOutput.txt

#Use stub input file
#./test/fsm 1 1 > test/qaOutput.txt

echo ">>> Comparing Outputs..."
diff test/qaOutput.txt test/qaOutputBaseline.txt

