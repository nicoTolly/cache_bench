#!/usr/bin/python3

import subprocess

# compute theorical and actual
# number of threads in full hits



Mlimit = 16.0
Climit = 25.0


output = subprocess.run(["sh", "./exec-load.sh"], stdout=subprocess.PIPE, universal_newlines=True)

nm = 0
nc = 0

lines = output.stdout.splitlines()

# threads are considered in hits if they
# have a throughput superior to Climit
# in full miss if their throughput is inferior
# to Mlimit
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
