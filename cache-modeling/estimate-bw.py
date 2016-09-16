import scipy.optimize
import numpy
import re

input_name = "data-dracula.txt"
input_name2 = "results2.txt"
input_name3 = "results3.txt"
input_name4 = "results4.txt"
def femp_interpol(datak, datav):
    coeff = numpy.polyfit(datak, datav, len(datak) - 1)
    diccoeff = [(i, coeff[len(datak) - i - 1]) for i in range(len(coeff))]
    def inter(x):
        return sum([ pow(x, i) * v for (i, v) in diccoeff])
    return inter

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
    delta = skey - fkey
    base = fkey
    x_rounded = (((xint - base) // delta ) * delta) + base
    lbda = (x - x_rounded  ) / float(delta)
    bwx = (1 - lbda) * data[x_rounded] + lbda * data[x_rounded + delta]
    return bwx


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





#get a function femp based on data passed as parameters
def fromdata(test):
    return lambda x: femp(x, test)

# returns the derivative of the function passed as parameters
# 
def derivate(f, delta):
    def func(x0) :  
         return ((f(x0 + delta) - f(x0)) / float(delta))
    return  func

def calculate_cache(lThreads, lBw):
    f = fromdata(data)
    #f = femp_interpol(datakeys, dataval)
    if ( not lThreads):
        return lBw
    elif (not lBw):
        bw = f(sum(lThreads))
        lBw.append(bw)
        lThreads.pop(0)
        return calculate_cache(lThreads, lBw)
    else:
        thrsum = sum(lThreads)
        lBwsum = sum(lBw)
        def func(x):
            return (f(x) * (x - thrsum) - (sum(lBw))*lThreads[0])
        xopt = scipy.optimize.newton_krylov(func, thrsum, f_tol = 1.0)
        lBw.append(f(xopt))
        lThreads.pop(0)
        return calculate_cache(lThreads, lBw)




dataf = open(input_name2, "r")
line = dataf.readline()
datakeys = []
dataval = []
data = dict()
while(line ):
    l = line.split()
    data[int(l[0])]= float(l[1])
    datakeys.append(int(l[0]))
    dataval.append(float(l[1]))
    line = dataf.readline()
dataf.close()
'''
fdata = fromdata(data)
fempD = derivate(fdata, 5)
n = 141
print("derivative in {} = {}\n".format(n, fempD(n)))
thrlist = get_threads()
tabThr = sorted(thrlist, reverse=True)
lBw = calculate_cache(tabThr, [])
print(lBw)
'''
fdata = fromdata(data)
#femp2 = femp_interpol(datakeys, dataval)
#print("{}\n".format(femp2(101)))

thrlist = get_threads()
tabThr = sorted(thrlist, reverse=True)
lBw = calculate_cache(tabThr, [])
print(lBw)
