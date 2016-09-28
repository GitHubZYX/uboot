#!/usr/bin/python

import os
import sys
import struct


if len(sys.argv) < 3:
	print 'No file specified.'
	sys.exit()

fin=sys.argv[1]
fout=sys.argv[2]

print fin
print fout


if os.path.exists(fout):
	os.remove(fout)

f=open(fin, 'rb')
fsize=os.path.getsize(fin)
fdata=f.read(fsize)

p1=str(fsize/8)+'Q'
byte_data=struct.unpack(p1, fdata)

out_data=''
for i in range(0, fsize/8):
	strtmp=str('%016x\n') %(byte_data[i])
	out_data=out_data+strtmp

f_out=open(fout, 'wb')
f_out.write(out_data)

f.close()
f_out.close()

