 for i in {200000..1600000..20000}; do echo $i | ./load | grep Throughput | column -s ':' -t | awk '{print $2}'; done
