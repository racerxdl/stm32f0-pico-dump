#!/usr/bin/env python

import argparse
import serial
import time
import struct

parser = argparse.ArgumentParser(description='Dump STM32 firmware')
parser.add_argument('--port', default="/dev/ttyACM0", help='serial port where raspberry pi pico is at. Usually /dev/ttyACM0')
parser.add_argument('output', help='file to write the firmware')

args = parser.parse_args()

print("Reading from {} to file {}".format(args.port, args.output))

with serial.Serial(args.port, 115200, timeout=5) as ser:
    # Wait for "Send anything to start..."
    line = ser.readline()
    if not b"Send anything to start..." in line:
        print("Expected {} got {}".format("Send anything to start...", line))
    else:
        print("Received ready. Starting dump")
        ser.write(b"S")
        time.sleep(1)
        line = ser.readline().decode("ascii")

        while not "Starting" in line:
            line = ser.readline().decode("ascii")
        print ("Received Starting")

        line = ser.readline().decode("ascii")
        with open(args.output, "wb") as f:
            while not "DONE" in line:
                if len(line) == 0:
                    continue
                addr, value = line.split(":")
                value = value.strip()
                print("Received {}: {}".format(addr, value))
                val = int(value, 16)
                f.write(struct.pack("<I", val))
                line = ser.readline().decode("ascii")
            print("Received DONE")