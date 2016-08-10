#!/usr/bin/python3
import re


# this script compute the number of threads
# supposed to be in full hits according to
# our model. Inputs are first cache size,
# then a blank line, every thread size (one
# per line) and then another blank line to confirm
# could be passed in standard input, like 
# cat inputs | ./calculate_cache.py

def get_cache():
    while(True):
        print("Enter cache size")
        s = input()
        if (re.match( '\d+', s)):
            return int(s)
        else:
            print("Cache size must be a number")


def get_threads():
    tab=[]
    print("Enter data size for each threads, press <Return> when finished")
    while(True):
        s = input()
        if (re.match( '\d+', s)):
            tab.append(int(s))
        elif (re.match('^$', s)):
            if (len(tab) == 0):
                print("You must at least declare one thread")
            else:
                return tab
        else:
            print("Enter number")

        



C = get_cache()
tab = get_threads()
N = len(tab)

Bm = 13
Bc = 40

# index of array
i = 0
# accumulators
res = 0
footprint = 0

sorted_tab = sorted(tab)
for i in range(N):
    footprint += sorted_tab[i]
    
    if (footprint + (N - i - 1) * Bm / Bc * sorted_tab[i]  > C ):
        i -= 1
        break

tab_str = ""

for a in sorted_tab:
    tab_str += str(a)
    tab_str += " "

print(tab_str)
print("{0} threads in full hits ".format(i+1))




