#!/usr/local/bin/gnuplot -persist



set terminal pngcairo size 800,600 enhanced font 'Verdana,12'
set output 'client_response_time_plot.png'

set title 'Number of Clients vs. Average Response Time'
set xlabel 'Number of Clients'
set ylabel 'Average Response Time (s)'

set style data linespoints


plot 'avgresponsevsM.txt' using 1:2 with linespoints title 'Average Response Time'


