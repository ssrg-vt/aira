set term png
set output "visualise.png"
set datafile separator ","

set title "2D PCA"
set xlabel "Best eigenvalue"
set ylabel "Second-best eigenvalue"
set grid
plot 'arch0.txt', 'arch1.txt', 'arch2.txt', 'arch3.txt', 'arch4.txt'
quit
