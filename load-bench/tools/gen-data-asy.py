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

nbBigThr = 4

rname += str(nbBigThr + 1)
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
	    res = re.search(".*execute on cpu 0.*\n.*Throughput : ([0-9]+\.[0-9]+)", output).group(1)
	    #print(res)
	    bw = float(res.strip())
            arr.append(bw)
        maxbw = max(arr)
        f1 = open(rname, 'a')
        f1.write('{0} {1}\n'.format(size, maxbw))
	print("finished benchmarking {0} bytes".format(size))



f = open(fname, 'w+')

#base array size, in number of double
base = 200000

bigThr = 20000000
f.write('{0}\n'.format(base))
for i in range(nbBigThr):
    f.write('{0}\n'.format(bigThr))
f.write('')

for i in range(30):
    f.seek(0)
    size = int(f.readline()) 
    gendata( 8 * size, f)
    f.seek(0)
    f.truncate()
    size_next_iter = size + 80000
    f.write('{0}\n'.format(size_next_iter))
    for i in range(nbBigThr):
        f.write('{0}\n'.format(bigThr))
    f.write('')
f.close()
os.remove(fname)
