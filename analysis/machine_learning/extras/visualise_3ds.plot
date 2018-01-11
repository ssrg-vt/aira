set term png
#set term wxt
set output "visualise_3ds.png"
set datafile separator ","

set title "2D PCA"
set xlabel "Best eigenvalue"
set ylabel "Second-best eigenvalue"
set zlabel "Speed-up"
set grid
splot 'arch0.txt', \
      'arch1.txt', \
      'arch2.txt', \
      'arch3.txt', \
      'arch4.txt'
quit
