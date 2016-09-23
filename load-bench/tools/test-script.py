#!/usr/bin/env python3

import math

# generate thread size according
# some polynomial law.
# Example : 4, 1000, 2 
# generates sizes of 1000 * 1 ^ 2 = 1000
# 1000 * 2 ^ 2 = 4000
# 1000 * 3 ^ 2 = 9000
# 1000 * 4 ^ 2 = 16000

def get_input():
    while(True):
        s = input()
        try:
            return float(s) 
        except:
            print("Must be a float")
    


def get_int():
    while(True):
        s = input()
        try:
            return int(s) 
        except:
            print("Must be a int")



#print("enter number of threads")
nb = get_int()
while(nb <= 0):
    print("Must be positive")
    nb = get_int()

#print("Enter coeff")
coef = get_int()
while(coef <= 0):
    print("coeff must be positive")
    coef = get_int()


#print("enter exponent")
exp = get_input()
while(exp <= 0):
    print("exponent must be positive")
    exp = get_input()

#print("\n")
l = [int(coef * pow(x, exp)) for x in range(1, nb+1)]

for f in l:
    print(str(f))

print("")

