import scipy.optimize
import numpy
import re
import sys
from random import randint

#input_name = "data-dracula.txt"
# datas from dracula benchmarks
input_name = "results1.txt"
input_name2 = "results2.txt"
input_name3 = "results3.txt"
input_name4 = "results4.txt"

# get a continuous and differentiable function
# from a data set
# not used in the code
def femp_interpol(datak, datav):
    coeff = numpy.polyfit(datak, datav, len(datak) - 1)
    diccoeff = [(i, coeff[len(datak) - i - 1]) for i in range(len(coeff))]
    def inter(x):
        return sum([ pow(x, i) * v for (i, v) in diccoeff])
    return inter

# this function takes a float and a sets of data
# approximate the value for input x by simply linking points
def femp(x, data):
    xint = int(x)
    sorted_keys = sorted(data.keys())
    fkey  = sorted_keys[0]
    skey  =  sorted_keys[1]
    lkey = sorted_keys[-1]
    if (xint < fkey):
        return data[fkey]
    if(xint > lkey):
        return data[lkey]
    #print("keys : {} {}\n".format(fkey , skey))
    # difference between two consecutives points
    delta = skey - fkey
    base = fkey
    x_rounded = (((xint - base) // delta ) * delta) + base
    lbda = (x - x_rounded ) / float(delta)
    bwx = (1 - lbda) * data[x_rounded] + lbda * data[x_rounded + delta]
    return bwx

# approximate a zero with a bisection
# less efficient than newton, used as a plan B
def bisect(f, x1, x2, n):
    if (( abs(x1 - x2) < 0.0001) or ( n > 10000) or (abs(f(x1)) < 0.001):
        return x1
    if( f(x1) * f(x2) < 0 ):
        y3 = f( (x1 + x2) / 2)
        if (y3 * f(x1) < 0):
            return bisect(f, x1, (x1 + x2) / 2, n + 1)
        else:
            return bisect(f, (x1 + x2) / 2, x2, n + 1)
    else:
        #case initial points happen to be of same sign
        if (randint(0, 100) < 50) :
            return bisect(f,x1 + x2, 2 * x2, n + 1)
        else:
            return bisect(f,x1 / 2, x2  / 2, n + 1)

# ask user for thread sizes input
# can be passed in standard input (supports EOF
# for terminating
def get_threads():
    tab=[]
    print("Enter data size for each threads, press <Return> when finished")
    s = sys.stdin.readline()
    while(s and (not re.match('^$', s))):
        s = s.strip()
        try:
            tab.append(int(s))
        except ValueError:
            print("Enter number")
        s = sys.stdin.readline()
            
    print(tab)
    return tab




#get a function femp based on data passed as parameters
def fromdata(test):
    return lambda x: femp(x, test)

# returns the derivative of the function passed as parameters
# 
def derivate(f, delta):
    def func(x0) :  
         return ((f(x0 + delta) - f(x0)) / float(delta))
    return  func

#take a reverse sorted thread sizes list and 
#returns the list of corresponding bandwidths
def calculate_cache(lThreads, lBw, data):
    f = fromdata(data)
    if ( not lThreads):
        return lBw
    elif (not lBw):
        bw = f(sum(lThreads))
        lBw.append(bw)
        lThreads.pop(0)
        return calculate_cache(lThreads, lBw)
    else:
        try:
            thrsum = sum(lThreads)
            lBwsum = sum(lBw)
            def func(x):
                return (f(x) * (x - thrsum) - (sum(lBw))*lThreads[0])
            xopt = scipy.optimize.newton_krylov(func, thrsum, f_tol = 1.0)
            lBw.append(f(xopt))
            lThreads.pop(0)
            return calculate_cache(lThreads, lBw)
        except ValueError:
            # newton approximation can fail when bw is too big
            # in this case we try to approach a solution
            # by a bisection method
            def func(x):
                return (f(x) * (x - thrsum) - (sum(lBw))*lThreads[0])
            xopt = bisect(func, lThreads[0], thrsum, 0)
            lBw.append(f(xopt))
            #bw = lBw[-1]
            #lBw.append(bw)
            lThreads.pop(0)
            return calculate_cache(lThreads, lBw)




dataf = open(input_name, "r")
line = dataf.readline()
datakeys = []
dataval = []
data = dict()
# retrieve data from files
while(line ):
    l = line.split()
    data[int(l[0])]= float(l[1])
    datakeys.append(int(l[0]))
    dataval.append(float(l[1]))
    line = dataf.readline()
dataf.close()


thrlist = get_threads()
# we want to iterate on threads in
# decreasing order 
tabThr = sorted(thrlist, reverse=True)
lBw = calculate_cache(tabThr, [], data)
print(lBw)
