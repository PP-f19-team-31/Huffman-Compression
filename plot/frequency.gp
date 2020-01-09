reset
set grid
set ylabel 'frequency(#char)'
set style fill solid
set title figtitle
set term png enhanced font 'Verdana,10'
set output outfile
set logscale y
set style histogram gap 0

plot [-2:257] infile with histogram title figtitle
