#!/usr/bin/python
# -*- coding: utf-8 -*-
import sys
import os

def usage():
    print("clear_logs.py")

def main(argv):
    '''
      clear logs directories
    '''
    dirlist = [ d for d in os.listdir(".") if d.endswith("sensor_log") ]
    for d in dirlist:
      loglist = [ f for f in os.listdir(d) if f.endswith(".log") ]
      for f in loglist:
        os.remove(os.path.join(d, f))
      dumplist = [ f for f in os.listdir(d) if f.endswith(".data") ]
      for f in dumplist:
        os.remove(os.path.join(d, f))

if __name__ == "__main__":
    main(sys.argv[1:])
