#!/usr/bin/python3

import configparser
import gzip
import sys
import os

output_path = sys.argv[3]
no_compress = False

#reading the first checkpoint
config0 = configparser.ConfigParser()
config0.optionxform=str
config0.read(sys.argv[1] + "/m5.cpt")

#reading the second checkpoint
config1 = configparser.ConfigParser()
config1.optionxform=str
config1.read(sys.argv[2] + "/m5.cpt")

#making the result ckeckpoint
config = configparser.ConfigParser()
config.optionxform=str
config.read(sys.argv[1] + "/m5.cpt")

#the curTick would be the max of the two
tick0 = int(config0['Globals']['curTick'])
tick1 = int(config1['Globals']['curTick'])
curTick = max(tick0, tick1)
config['Globals']['curTick'] = str(curTick)

#make [system.cpu.xc.1]
config['system.cpu.xc.1'] = config1['system.cpu.xc.0']


#make [system.cpu.workload0]
config.remove_section('system.cpu.workload')
config["system.cpu.workload0"] = config0["system.cpu.workload"]
config["system.cpu.workload1"] = config1["system.cpu.workload"]


#fix [system.cpu.workload.vmalist]
config.remove_section('system.cpu.workload.vmalist')
config["system.cpu.workload0.vmalist"] = config0["system.cpu.workload.vmalist"]
config["system.cpu.workload1.vmalist"] = config1["system.cpu.workload.vmalist"]

#fix system.cpu.workload.vmalist.vma?
size_vmalist0 =  int(config0["system.cpu.workload.vmalist"]['size'])
for i in range (size_vmalist0):
    vma = "system.cpu.workload.vmalist.Vma%d" % i
    config.remove_section(vma)
    config['system.cpu.workload0.vmalist.Vma%d' % i] = \
        config0['system.cpu.workload.vmalist.Vma%d' % i]

size_vmalist1 =  int(config1["system.cpu.workload.vmalist"]['size'])
for i in range (size_vmalist1):
    config['system.cpu.workload1.vmalist.Vma%d' % i] = \
        config1['system.cpu.workload.vmalist.Vma%d' % i]


#same thing for [system.cpu.workload.ptable]
config.remove_section('system.cpu.workload.ptable')
config["system.cpu.workload0.ptable"] = config0["system.cpu.workload.ptable"]
config["system.cpu.workload1.ptable"] = config1["system.cpu.workload.ptable"]


##############################################################################
#fix [system]
#config['system']['quiesceEndTick_1'] = config1['system']['quiesceEndTick_0']
pagePtr0 = int(config0['system']['pagePtr'])
pagePtr1 = int(config1['system']['pagePtr'])
config['system']['pagePtr'] = str(pagePtr0 + pagePtr1)


memory_size = int(config['system.physmem.store0']['range_size'])
paddr_offset_t1 = pagePtr0 * 4 * 1024


##############################################################################
#fix [system.cpu.workload.ptable.Entry?]

size_ptable0 =  int(config0["system.cpu.workload.ptable"]['size'])
print ("ptable size t0: ", size_ptable0)
for i in range (size_ptable0):
    ptable = "system.cpu.workload.ptable.Entry%d" % i
    config.remove_section(ptable)
    config['system.cpu.workload0.ptable.Entry%d' % i] = \
        config0['system.cpu.workload.ptable.Entry%d' % i]

size_ptable1 =  int(config1["system.cpu.workload.ptable"]['size'])
print ("ptable size t1: ", size_ptable1)
for i in range (size_ptable1):
    config['system.cpu.workload1.ptable.Entry%d' % i] = \
        config1['system.cpu.workload.ptable.Entry%d' % i]
    paddr =  int(config['system.cpu.workload1.ptable.Entry%d' % i]['paddr'])
    config['system.cpu.workload1.ptable.Entry%d' % i]['paddr'] = \
        str( paddr + paddr_offset_t1)

##############################################################################
#fix itb
if config0.has_section('system.cpu.itb'):
    size_itb0 =  int(config0["system.cpu.itb"]['_size'])
    size_itb1 =  int(config0["system.cpu.itb"]['_size'])
    sum_itb   =  size_itb0 + size_itb1
    config['system.cpu.itb']['_size'] = str(sum_itb)

    maxLRU = 0
    print ("offset for paddr:" + str(paddr_offset_t1))
    for i in range (size_itb0):
        config['system.cpu.itb.Entry%d' % i]['asn'] = '0'
        config['system.cpu.itb.Entry%d' % i]['lruSeq'] = str(maxLRU)
        maxLRU = maxLRU + 1
    for i in range (size_itb0, sum_itb):
        config['system.cpu.itb.Entry%d' %i] = \
            config1['system.cpu.itb.Entry%d' % (i-size_itb0)]
        config['system.cpu.itb.Entry%d' % i]['asn'] = '1'
        paddr = int(config['system.cpu.itb.Entry%d' % i]['paddr'])
        config['system.cpu.itb.Entry%d' % i]['paddr'] = \
            str(paddr_offset_t1 + paddr)
        config['system.cpu.itb.Entry%d' % i]['lruSeq'] = str(maxLRU)
        maxLRU = maxLRU + 1

    config['system.cpu.itb']['lruSeq'] = str(maxLRU)

##############################################################################
#fix dtb
if config0.has_section('system.cpu.dtb'):
    size_dtb0 =  int(config0["system.cpu.dtb"]['_size'])
    size_dtb1 =  int(config0["system.cpu.dtb"]['_size'])
    sum_dtb   =  size_dtb0 + size_dtb1
    config['system.cpu.dtb']['_size'] = str(sum_dtb)

    maxLRU = 0
    print ("offset for paddr:" + str(paddr_offset_t1))
    for i in range (size_dtb0):
        config['system.cpu.dtb.Entry%d' % i]['asn'] = '0'
        config['system.cpu.dtb.Entry%d' % i]['lruSeq'] = str(maxLRU)
        maxLRU = maxLRU + 1
    for i in range (size_dtb0, sum_dtb):
        config['system.cpu.dtb.Entry%d' %i] = \
            config1['system.cpu.dtb.Entry%d' % (i-size_dtb0)]
        config['system.cpu.dtb.Entry%d' % i]['asn'] = '1'
        paddr = int(config['system.cpu.dtb.Entry%d' % i]['paddr'])
        config['system.cpu.dtb.Entry%d' % i]['paddr'] = \
            str(paddr_offset_t1 + paddr)
        config['system.cpu.dtb.Entry%d' % i]['lruSeq'] = str(maxLRU)
        maxLRU = maxLRU + 1

    config['system.cpu.dtb']['lruSeq'] = str(maxLRU)


##############################################################################
#fix [system.cpu.isa]
config.remove_section('system.cpu.isa')
config['system.cpu.isa0'] = config0['system.cpu.isa']
config['system.cpu.isa1'] = config1['system.cpu.isa']


##############################################################################
#fix [syste.cpu.interrupts]
config.remove_section('system.cpu.interrupt')
config['system.cpu.interrupts0'] = config0['system.cpu.interrupts']
config['system.cpu.interrupts1'] = config1['system.cpu.interrupts']





with open(output_path + '/m5.cpt', 'w') as configfile:    # save
    config.write(configfile)
############################################################################
# memory
src_store0_path =  sys.argv[1] + "/" +\
     config0['system.physmem.store0']['filename']
src_store1_path =  sys.argv[2] + "/" +\
     config1['system.physmem.store0']['filename']


agg_mem_file = open(output_path + "/system.physmem.store0.pmem", "wb+")

if not no_compress:
    merged_mem = gzip.GzipFile(fileobj= agg_mem_file, mode="wb")

print ("pages to be read: ", pagePtr0)
f = open(src_store0_path, "rb")
gf = gzip.GzipFile(fileobj=f, mode="rb")

x = 0
while x < pagePtr0:
    bytesRead = gf.read(4*1024)
    if not no_compress:
        merged_mem.write(bytesRead)
    else:
        agg_mem_file.write(bytesRead)
    x += 1
gf.close()
f.close()

print ("pages to be read: ", pagePtr1)
f = open(src_store1_path,"rb")
gf = gzip.GzipFile(fileobj=f, mode="rb")

x = 0
while x < pagePtr1:
    bytesRead = gf.read(4*1024)
    if not no_compress:
        merged_mem.write(bytesRead)
    else:
        agg_mem_file.write(bytesRead)
    x += 1
gf.close()
f.close()

pagePtr = (pagePtr0 + pagePtr1)
file_size = pagePtr * 4 * 1024
b = bytearray(0)
dummy_data = b.zfill(4096)
while file_size < memory_size:
    if not no_compress:
        merged_mem.write(dummy_data)
    else:
        agg_mem_file.write(dummy_data)
    file_size += 4 * 1024
    pagePtr += 1

print ("WARNING: ")
print ("Make sure the simulation using this checkpoint has at least ")
print (pagePtr, "x 4K of memory")
config["system.physmem.store0"]["range_size"] = str(pagePtr * 4 * 1024)


if not no_compress:
    merged_mem.close()
    agg_mem_file.close()
else:
    agg_mem_file.close()


