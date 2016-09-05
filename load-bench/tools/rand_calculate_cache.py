#!/usr/bin/python3
# vim: tabstop=8 expandtab shiftwidth=4 softtabstop=4
import re
import math


# this script compute the number of threads
# supposed to be in full hits according to
# our model. Inputs are first cache size,
# then a blank line, every thread size (one
# per line) and then another blank line to confirm
# could be passed in standard input, like 
# cat inputs | ./calculate_cache.py
'''
def get_cache():
    while(True):
        print("Enter cache size")
        s = input()
        if (re.match( '\d+', s)):
            return int(s)
        else:
            print("Cache size must be a number")
'''


        





Bm = 20.0
Bc = 40.0
m_speed = Bm / 4 

data_size_base = 500000.0
cache_size = 6000000.0


def get_bw(data_s, coeff) :
    #probability that a data is in cache
    if (data_s * 4 < cache_size):
        pc = 1.0
    else:
        pc = (cache_size - coeff ) / (data_s * 4 )
    m_speed = Bm / 4 
    return (Bc * pc + m_speed * (1 - pc))

def get_bw_exp(data_s) :
    inter_val = (cache_size - data_s*4) / 800000
    pc = min(1, math.exp(inter_val))
    return (Bc * pc + m_speed * (1 - pc))

'''
coeff_LRU = 0
for i in range(5):
    print("coeff LRU = {0}".format(coeff_LRU))
    data_size = data_size_base
    for i in range(40):
        bw = get_bw(data_size, coeff_LRU)
        data_size = data_size + 50000
        print("{0} {1}".format(data_size, bw))
    coeff_LRU += 500000
'''

data_size = data_size_base
for i in range(40):
    bw = get_bw_exp(data_size )
    data_size = data_size + 50000
    print("{0} {1}".format(data_size, bw))
