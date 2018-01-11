#!/bin/bash

###############################################################################
# Config
###############################################################################

CMD="../train"
PCA="10 11 12 13 14 15 16"
HIDDEN_LAYER="9 10 11 12 13 14 15 16"
RATE="0.02 0.04 0.06 0.08 0.1 0.12"
MOMENTUM="0.1 0.2 0.5"
ITER=1500

MLROOT="`readlink -f ../`"
RESULTS="./nn-shape-exploration"

###############################################################################
# Driver
###############################################################################

if [ ! -e "../train" ]; then
	echo "Please build training program before running this script"
	exit 1
fi

if [ "$1" == "" ]; then
	echo "Please supply a directory containing training data"
	exit 1
fi
DATA="$1"

if [ "$2" != "" ]; then
	RESULTS=$2
fi

# Gather training data
training_data=""
for file in `ls $DATA/*`; do
	training_data="$training_data --data $file"
done

# Train models
mkdir -p $RESULTS
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$MLROOT/build
for pca in $PCA; do
	for hidden_layer in $HIDDEN_LAYER; do
		for rate in $RATE; do
			for momentum in $MOMENTUM; do
				echo "$pca $hidden_layer $rate $momentum"
				$CMD $training_data \
					--pca $pca \
					--iter $ITER \
					--layer $hidden_layer \
					--rate $rate \
					--momentum $momentum > $RESULTS/${pca}_${hidden_layer}_${rate}_${momentum}.txt &
			done
		done
		wait
	done
done

