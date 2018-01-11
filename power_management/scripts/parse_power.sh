#!/bin/bash

if [ "$1" == "-h" ] || [ "$1" == "--help" ]; then
	echo "Usage: ./parse_power.sh <input directory> <cpu | gpu | clean>"
	exit 0
fi

# Simple frontend for parse_power.py, where the real magic happens
if [ "$2" == "gpu" ]; then
	echo "Parsing GPU lines from $1"
	for f in `ls $1/*`; do
		if [[ "$f" != *.csv ]]; then
			echo "Skipping $f"
			continue
		fi
		echo "   +++ $f"
		./parse_power.py -i $f -o ${f/.csv/_power.csv} \
			-l "5,12.0,0.1,2.0,12V GPU (ATX 6-pin)" \
			-l "6,12.0,0.1,2.0,12V GPU (ATX 8-pin)" \
			-l "7,12.0,0.1,2.0,12V GPU (PCIe)" \
			-r -a 10 &> ${f/.csv/_summary.txt}
	done
elif [ "$2" == "cpu" ]; then
	echo "Parsing CPU lines from $1"
	for f in `ls $1/*`; do
		if [[ "$f" != *.csv ]]; then
			echo "Skipping $f"
			continue
		fi
		echo "   +++ $f"
		./parse_power.py -i $f -o ${f/.csv/_power.csv} \
			-l "0,12.0,0.1,2.0,12V CPU" \
			-l "1,12.0,0.1,2.0,12V motherboard" \
			-l "2,5.0,0.1,1.0,5V motherboard" \
			-l "4,3.3,0.1,1.0,3.3V motherboard" \
			-l "7,12.0,0.1,-2.0,12V GPU (PCIe) - subtracted" \
			-r -a 10 &> ${f/.csv/_summary.txt}
	done
else
	echo "Cleaning $1"
	rm -f $1/*_power.csv
	rm -f $1/*_summary.txt
fi
