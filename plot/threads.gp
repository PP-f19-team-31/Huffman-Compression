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
plot enfile using 1:2 w linespoints lw 1 title 'english', \
    zhfile using 1:2 w linespoints lw 1 title 'chinese', \
    binfile using 1:2 w linespoints lw 1 title 'binary'
