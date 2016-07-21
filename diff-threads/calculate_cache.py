#!/usr/bin/python3
import re



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

        


tab = [1, 2, 3, 10, 17, 19]

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
if(i == -1):
    print("Every thread is in full miss")
else:
    print("Last thread with full hits is number {0} ".format(i+1))



