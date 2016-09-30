#!/usr/bin/python

import fileinput
import re



lines = []

for l in fileinput.input():
    lines.append(l)

tab = lines[0].split()

nTh = tab[0]
N = tab[1]
K = tab[2]

mean = 8*sum(map(float, lines[1:]))/ len(lines[1:])
# printing number of threads, number of double loaded per thread, number of bytes globally
# loaded, and global bandwidth
print(nTh + ', ' + N + ', '+ str(int(N) * int(nTh) * 8)  + ', ' + str(mean))
