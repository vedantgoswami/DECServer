# Set the terminal type and output file format (e.g., PNG).
set terminal pngcairo size 800,600 enhanced font 'Verdana,12'
set output 'client_throughput_plot.png'

# Set the title and labels for the axes.
set title 'Number of Clients vs. Throughput'
set xlabel 'Number of Clients'
set ylabel 'Throughput (requests per second)'

# Set the style of the data points and lines (if needed).
set style data linespoints

# Plot the data from the 'avgresponsevsM.txt' file using columns 1 and 3.
plot 'avgresponsevsM.txt' using 1:3 with linespoints title 'Throughput'
