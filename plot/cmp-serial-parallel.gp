reset
set grid
set ylabel 'time(sec)'
set style fill solid
set title figtitle
set ydata time
set timefmt "%M:%S"
set format y "%M:%S"
set term png enhanced font 'Verdana,10'
set output outfile 
set logscale y

plot [:][:100]infile using 2:xtic(1) with histogram title 'serial', \
'' using 3:xtic(1) with histogram title '#t=4'  , \
'' using 4:xtic(1) with histogram title '#t=8'  , \
'' using 5:xtic(1) with histogram title '#t=16'  , \
'' using 6:xtic(1) with histogram title '#t=32'  , \
'' using 7:xtic(1) with histogram title '#t=64'
