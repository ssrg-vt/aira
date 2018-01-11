#!/bin/bash

MODE="$1"
EXPERIMENT="$2"
MAX_ARCH=4 # if increasing extend extras/*.plot

shift ; shift
source experiment.sh &> /dev/null

function mode {
  [[ "$MODE" == "--2d" ]] && echo
  [[ "$MODE" == "--3d" ]] && echo --3d
  [[ "$MODE" == "--3d-speedup" ]] && echo --3d-speedup
  [[ "$MODE" == "--interactive" ]] && echo --3d
  [[ "$MODE" == "--interactive-speedup" ]] && echo --3d-speedup
}

# Overrides the do_run in experiment.sh, i.e. don't run the experiment, do some
# visualisation on the same data and arguments!
function do_run {
  export LD_LIBRARY_PATH="${LD_LIBRARY_PATH}:./build"
  for i in `seq 0 $MAX_ARCH` ; do
    ./train $* --visualise `mode` | grep arch${i}_suitability > arch${i}.txt
  done
  return 0
}

if [[ "$MODE" == "--2d" ]] ; then
  run $EXPERIMENT
  gnuplot extras/visualise.plot
elif [[ "$MODE" == "--3d" ]] ; then
  run $EXPERIMENT
  gnuplot extras/visualise_3d.plot
elif [[ "$MODE" == "--3d-speedup" ]] ; then
  run $EXPERIMENT
  gnuplot extras/visualise_3ds.plot
elif [[ "$MODE" == "--interactive" ]] ; then
  run $EXPERIMENT
  cat extras/visualise_3d.plot | sed -e 's/png/wxt/' | gnuplot -persist
  rm visualise_3d.wxt
elif [[ "$MODE" == "--interactive-speedup" ]] ; then
  run $EXPERIMENT
  cat extras/visualise_3ds.plot | sed -e 's/png/wxt/' | gnuplot -persist
  rm visualise_3ds.wxt
else
  echo "Usage: $0 MODE EXPERIMENT"
  echo "    MODE: '--2d', '--3d[-speedup]', or '--interactive[-speedup]'"
  ./experiment.sh | grep -v Usage
  exit 1
fi

rm arch[0-9]*.txt
