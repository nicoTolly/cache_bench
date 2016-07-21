#!/usr/bin/python3

import subprocess


Mlimit = 16.0
Climit = 25.0


output = subprocess.run(["sh", "./exec-load.sh"], stdout=subprocess.PIPE, universal_newlines=True)

nm = 0
nc = 0

lines = output.stdout.splitlines()

for line in lines:
    if (float(line) > Climit):
        nc += 1
    elif (float(line) < Mlimit):
        nm += 1


print("{0} threads in full hits".format(nc))
print("{0} threads in full miss".format(nm))

output = subprocess.run(["sh", "./eval-cache.sh"], stdout=subprocess.PIPE, universal_newlines=True)


expect_hits = int(output.stdout)
print("expected {0} full hits threads".format(expect_hits))
