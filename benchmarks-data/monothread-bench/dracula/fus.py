#!/usr/bin/python3


inname = "inputs.txt"
outname = "outputs.txt"
datname = "data.txt"

inf = open(inname, 'r')
outf = open(outname, 'r')
datf = open(datname, 'w+')

inf.seek(0)
outf.seek(0)
datf.seek(0)

while(1):
    linein = inf.readline().strip()
    lineout = outf.readline().strip()
    if (not linein or not lineout):
        break
    inpt = int(linein)
    outpt = float(lineout)
    datf.write('{0}  {1}\n'.format(inpt, outpt))

inf.close()
outf.close()
datf.close()

