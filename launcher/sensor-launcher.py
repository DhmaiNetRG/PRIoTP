#!/usr/bin/python
# -*- coding: utf-8 -*-
import sys
import subprocess
import os
import shutil
import random
import signal
import time

spawns=[] #list of subprocesses spawned 

def usage():
    print( "sensor-launcher.py  <server_ip> <server_sensor_port> <sim_time> <num_of_sensors> ")

def signal_handler(signal, frame):
  print( 'Shutting down Launcher...')
  #probably need to close all the forked subprocesses?
  sys.exit(0)

def main(argv):
  ipaddr="localhost"
  port="5000"
  client_port="5001"
  sim_time=10 #seconds
  N=4 #or >4 (number of sensors)
  
#   if(len(argv)>=1):
  if(len(argv)>=3):
    ipaddr=argv[0]
    port=argv[1]
    sim_time=int(argv[2])
    if(len(argv)>=4):
      N=int(argv[3])
   
  if len(argv)!=0:
    if (argv[0] == "-h" or argv[0]=="--help"):
      usage()
      sys.exit(0)
  else:
    print( "using Launcher defaults =>")

  print( "ip",ipaddr,"and port:",port)
  print( "sim_time",sim_time)

  start_time = round(time.time(), 3)
  #[STEP0]:
  #creating sensor.list
  sensorFile = open('./sensor.list', 'wb')

  sensors=["device", "temp", "gps", "camera"]
  
  print( range(N))
  for i in range(N): 
    #choose randomly a sensor from the list
    if (N>4):
      sensor_type=random.choice(sensors)
      sid=str(random.randint(10000,99999))
    else:
      sensor_type=sensors[i]
      sid=str(i)
    
    sensorFile.write(str(sensor_type+"_"+sid+"\n").encode('utf-8'))

  #[STEP1]: starting sensors
  sensorFile.close
  sensorFile = open('./sensor.list', 'r')
  for i in range(N): 
    sensor_type, sid = sensorFile.readline().strip().split('_')
    print( "starting",(sensor_type+sid),"...")
    spawns.append(subprocess.Popen(["python3","sensor.py", sensor_type, ipaddr, port, sid],stdout=False,shell=False))
   
  
  sensorFile.close
  
  
  time.sleep(sim_time)
  print( "killing processes after", (time.time()-start_time))
  for s in spawns:
    #print( s.pid
    os.kill(s.pid, signal.SIGINT)

if __name__ == "__main__":
  signal.signal(signal.SIGINT, signal_handler)
  main(sys.argv[1:])
