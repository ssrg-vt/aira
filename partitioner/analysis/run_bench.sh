#!/bin/bash

DATA=W
BENCH="FT CG EP IS MG SP UA BT LU DC"

[ $# -gt 0 ] && BENCH=$@

LIBM="bench/popcorn_libm_xeon.c"

for a in $BENCH ; do
  DIR="bench/${a}.${DATA}"
  PAR="bench/${a}.parallel.txt"
  LOG="bench/${a}.${DATA}.log"
  POPCORN="bench/${a}.popcorn.txt"
  GRAPH="bench/${a}.pdf"

  ./run.sh "$PAR" "$DIR"/*.c | tee "$LOG"
  cat "$LOG" | grep '#|  ' | sed -e 's/#|  //' > "$POPCORN"
  mv graph.pdf "$GRAPH"

  [[ "$a" == "DC" ]] && rm -f ADC.*
done
