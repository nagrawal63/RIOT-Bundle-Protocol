#!/usr/bin/python3
import serial
import csv
import os

myhost = os.uname()[1]
ser = serial.Serial(port = '/dev/ttyACM0', baudrate=115200)
ser.flushInput()
filename = 'out_'+myhost+'.csv'
csv_file = open(filename,'w')
with csv_file :
    col_names = ['System time', 'Bundles in storage', 'Bundles sent', 'Bundles received', 'Bundles forwarded', 'Bundles retransmitted', 'Bundles Delivered', 'Ack sent', 'Ack received', 'Discovery sent', 'Discovery received']
    writer = csv.DictWriter(csv_file, fieldnames=col_names)
    writer.writeheader()
    while True:
        read_bytes = ser.readline()
        print(read_bytes)
        try :
            ser_bytes = read_bytes.decode("utf-8")
            print(ser_bytes)
            if "#*#*" in ser_bytes:
                arr = ser_bytes.split(",")
                print(arr)
                writer.writerow({'System time': arr[1], 'Bundles in storage': arr[2], 'Bundles sent': arr[3], 'Bundles received': arr[4], 'Bundles forwarded': arr[5], 'Bundles retransmitted': arr[6], 'Bundles Delivered': arr[7], 'Ack sent': arr[8], 'Ack received': arr[9], 'Discovery sent': arr[10], 'Discovery received': arr[11]})
                print("-------------------------------------------------------")
        except (UnicodeDecodeError, AttributeError):
            pass
