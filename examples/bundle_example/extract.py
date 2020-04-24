#!/usr/bin/python3

import csv

logs = open('log.txt','r')
csv = open('out.csv','w')
lines = logs.readlines()

with csv:
    col_names = ['System time', 'Bundles in storage', 'Bundles sent', 'Bundles received', 'Bundles forwarded', 'Bundles retransmitted', 'Bundles Delivered', 'Ack sent', 'Ack received']
    writer = csv.DictWriter(csv, fieldnames=col_names)
    writer.writeheader()
    for line in lines:
        if "#*#*" in line:
            arr = line.split(",")
            writer.writerow({'System time': arr[1], 'Bundles in storage': arr[2], 'Bundles sent': arr[3], 'Bundles received': arr[4], 'Bundles forwarded': arr[5], 'Bundles retransmitted': arr[6], 'Bundles Delivered': arr[7], 'Ack sent': arr[8], 'Ack received': arr[9]})
        
