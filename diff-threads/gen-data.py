#!/usr/bin/python3

import math
import subprocess

fname = 'thread-sizes.txt'
rname = "results6.txt"

nbThr = 4

def gendata(size, f):
    arr = []
    with open(fname, 'r') as f:
        for j in range(15):
            f.seek(0)
            load = subprocess.Popen(["./load"],  stdin=f, stdout=subprocess.PIPE)
            awk = subprocess.Popen(["awk", "/throughput/ {print $4}"],  stdin=load.stdout, stdout=subprocess.PIPE, universal_newlines=True )
            load.stdout.close()
            output = awk.communicate()[0]
            load.wait()
            res = float(output.strip())
            arr.append(res)
        mean = sum(arr) / len(arr)
        f1 = open(rname, 'a')
        f1.write('{0} {1}\n'.format(size, mean))



f = open(fname, 'w+')

nbset = 6144 * 1024 / 12
for i in range(nbThr):
    f.write('{0}\n'.format(nbset))
f.write('')

for i in range(12):
    f.seek(0)
    size = int(f.readline()) 
    gendata(nbThr * 8 * size, f)
    f.seek(0)
    f.truncate()
    for i in range(nbThr):
        f.write('{0}\n'.format(size * 2))
    f.write('')
f.close()
