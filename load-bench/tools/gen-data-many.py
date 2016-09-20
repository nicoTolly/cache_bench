#!/usr/bin/python3

import math
import subprocess
import os
import re

# Generate some datas from benchmark
# with increasing sizes of array in inputs
# store them (thread size then throughput) into results<nb_threads>.txt

fname = 'thread-sizes.txt'
rname = "results"
binpath = "../load"

nbThr = 5

rname += str(nbThr)
rname += ".txt"

def gendata(size, f):
    arr = []
    with open(fname, 'r') as f:
        for j in range(10):
            f.seek(0)
            load = subprocess.Popen([binpath],  stdin=f, stdout=subprocess.PIPE)
            load.wait()
	    output = load.communicate()[0]
	    #load.stdout.close()
	    #res = re.search(".*execute on cpu 0.*\n.*Throughput : ([0-9]+\.[0-9]+)", output).group(1)
	    res = re.search(".*Global throughput : ([0-9]+\.[0-9]+)", output).group(1)
	    #print(res)
	    bw = float(res.strip())
            arr.append(bw)
        maxbw = max(arr)
        f1 = open(rname, 'a')
        f1.write('{0} {1}\n'.format(size*nbThr, maxbw/nbThr))
	print("finished benchmarking {0} bytes".format(size))



f = open(fname, 'w+')

#base array size, in number of double
base = 50000
#stride
step = 20000

for i in range(nbThr):
    f.write('{0}\n'.format(base))

for i in range(170):
    f.seek(0)
    size = int(f.readline()) 
    gendata( 8 * size, f)
    f.seek(0)
    f.truncate()
    size_next_iter = size + step
    for i in range(nbThr):
        f.write('{0}\n'.format(size_next_iter))
f.close()
os.remove(fname)
