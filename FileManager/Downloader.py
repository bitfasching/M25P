#!/usr/bin/python
#encoding: utf-8

# floating point division by default
from __future__ import division

# import tools
import sys
import serial
import time

# config: chunk size for data transfer
chunkSize = 256

# config: serial speed for download
baudRate = 250000

# config: timeout for serial reads (seconds)
serialTimeout = 0.1

# device's ready message
deviceReadyMessageDownload = "READY"

# newline to separate from command prompt
print

# show info on how to use
def printUsage():
    print "Usage: % FILE [DEVICE]" % sys.argv[0]
    print
    exit(1)

# format time as minutes and seconds
def toMinutes( seconds ):
    return "%02dm%02ds" % ( seconds/60, round(seconds)%60 )

# parse required arguments
if len(sys.argv) > 1:
    filename = sys.argv[1].strip()
else:
    printUsage()

# optional argument
if len(sys.argv) > 2:
    deviceId = sys.argv[2].strip()
else:
    deviceId = '/dev/ttyUSB0'

try:

    # open serial connection (standard 8N1, one second timeout)
    with serial.Serial( deviceId, baudRate, bytesize=8, parity='N', timeout=serialTimeout ) as device:

        # say hello
        print "Flash Downloader"
        print "  → Connected to:", deviceId
        print "  → Waiting for device to react..."

        # wait for device to prepare the serial port
        deviceHello = ""
        while not deviceHello:
            deviceHello = device.readline()
            pass

        print "    Ready. Device says: \"%s\"" % deviceHello.strip()

        print "  → Preparing to download. Erasing..."
        device.write( 'D' )

        # wait for device
        while not deviceReadyMessageDownload in device.readline():
            pass

        print "  → Downloading..."

        # open file…
        with open( filename, 'r' ) as inputFile:

            # …and read whole file (we got the RAM for this, haha)
            data = inputFile.read()

        # position in data
        dataIndex = 0

        # iterate in chunks over data
        while dataIndex < len(data):

            # extract chunk of data
            chunk = data[ dataIndex : dataIndex+chunkSize ]

            # send to device
            device.write( chunk )

            # increment index (this equals the number of transferred bytes btw)
            dataIndex = dataIndex + len(chunk)

            # show status
            print "\r    %d Bytes" % dataIndex, " (%d%%)" % (dataIndex/len(data)*100),

            # wait for device before going on
            while not deviceReadyMessageDownload in device.readline():
                pass

        print "\n    Done!"
        print

        while True:
            line = device.readline()
            if line:
                print line.strip()

# show message upon Ctrl+C
except KeyboardInterrupt:
    print
    print "Aborted!"
    print
    exit(1)

# show message for serial exception
except serial.SerialException as error:
    print "Serial exception:",
    print error
    print
    exit(1)

print
exit(0)
