#!/usr/bin/env python

import math
import subprocess
import os

# Generate some datas from benchmark
# with increasing sizes of array in inputs
# store them (thread size then throughput) into results<nb_threads>.txt

fname = 'thread-sizes.txt'
rname = "results"
binpath = "../load"

nbThr = 1

rname += str(nbThr)
rname += ".txt"

def gendata(size, f):
    arr = []
    with open(fname, 'r') as f:
        for j in range(15):
            f.seek(0)
            load = subprocess.Popen([binpath],  stdin=f, stdout=subprocess.PIPE)
            awk = subprocess.Popen(["awk", "/throughput/ {print $4}"],  stdin=load.stdout, stdout=subprocess.PIPE, universal_newlines=True )
            load.stdout.close()
            output = awk.communicate()[0]
            load.wait()
            res = float(output.strip())
            arr.append(res)
        maxbw = max(arr)
        f1 = open(rname, 'a')
        f1.write('{0} {1}\n'.format(size, maxbw))



f = open(fname, 'w+')

#cache is 12-ways associative
# so the number of set is size of cache / 12
nbset = int(6144 * 1024 / 12)
#base array size, in number of double
base = int(nbset / nbThr / 8)

for i in range(nbThr):
    f.write('{0}\n'.format(base))
f.write('')

for i in range(10):
    f.seek(0)
    size = int(f.readline()) 
    gendata(nbThr * 8 * size, f)
    f.seek(0)
    f.truncate()
    size_next_iter = size * 2
    for i in range(nbThr):
        f.write('{0}\n'.format(size_next_iter))
    f.write('')
f.close()
os.remove(fname)
