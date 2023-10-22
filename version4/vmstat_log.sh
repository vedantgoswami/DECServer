#!/bin/bash

interval=1

# Output file
output_file="./logs/mpstat_$1.log"

# Clear the output file or create it if it doesn't exist
> "$output_file"

mpstat -P ALL 1 >> "$output_file"


