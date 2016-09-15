import scipy
input_name = "data.txt"
def femp(x, data):
    sorted_keys = sorted(data.keys())
    fkey  = sorted_keys[0]
    skey  =  sorted_keys[1]
    #print("keys : {} {}\n".format(fkey , skey))
    delta = skey - fkey
    base = fkey
    x_rounded = (((x - base) / delta ) * delta) + base
    print("x rounded down : {} \n".format(x_rounded))
    lbda = (x - x_rounded  ) / float(delta)
    bwx = (1 - lbda) * data[x_rounded] + lbda * data[x_rounded + delta]
    return bwx







def fromdata(test):
    return lambda x: femp(x, test)

def derivate(f, delta):
    def func(x0) :  
         return ((f(x0 + delta) - f(x0)) / float(delta))
    return  func



dataf = open(input_name, "r")
line = dataf.readline()
data = dict()
while(line ):
    l = line.split()
    data[int(l[0])]= int(l[1])
    line = dataf.readline()

fdata = fromdata(data)
fempD = derivate(fdata, 5)
n = 141
print("derivative in {} = {}\n".format(n, fempD(n)))


