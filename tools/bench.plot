set boxwidth 0.5
set style fill solid
set nokey
set ylabel "Time (s)"
set term png size 640,480
set output "bench.png"
set format x "\n"
set format y "%.1f"
set yrange [0:]
# set logscale y 2
plot "bench.dat" using 2:xtic(1) with boxes lc rgb "blue"
