 for i in {300000..1000000..20000}; do echo $i | ./load | grep Throughput; done
