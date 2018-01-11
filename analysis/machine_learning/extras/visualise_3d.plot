set term png
#set term wxt
set output "visualise_3d.png"
set datafile separator ","

set title "3D PCA"
set xlabel "Best eigenvalue"
set ylabel "Second-best eigenvalue"
set zlabel "Third-best eigenvalue"
set grid
splot 'arch0.txt', \
      'arch1.txt', \
      'arch2.txt', \
      'arch3.txt', \
      'arch4.txt'
quit
