reset
set grid

set ylabel 'time(sec)'
set ydata time
set logscale y
set timefmt "%M:%S"
set format y "%M:%S"
set title figtitle
set term png enhanced font 'Verdana,10'

set output outfile

plot t1 using 1:2 w linespoints lw 1 title '1 thread', \
     t4 using 1:2 w linespoints lw 1 title '4 threads', \
     t8 using 1:2 w linespoints lw 1 title '8 threads', \
     t16 using 1:2 w linespoints lw 1 title '16 threads', \
     t32 using 1:2 w linespoints lw 1 title '32 threads', \
     t64 using 1:2 w linespoints lw 1 title '64 threads'
