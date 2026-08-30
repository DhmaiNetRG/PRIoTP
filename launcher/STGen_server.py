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
    print("STGen_server.py <clients_configuration_file> <server_ip> <server_sensor_port> <server_client_port> <sim_time>")

def signal_handler(signal, frame):
    print('Shutting down Server...')
    for s in spawns:
        try:
            os.kill(s.pid, signal.SIGINT)
        except:
            pass
    sys.exit(0)

def main(argv):
    ipaddr="localhost"
    port="5000"
    client_port="5001"
    client_config="./PRTP/conf/test.conf"
    sim_time=10 #seconds
    
    if(len(argv)>=1):
        client_config=argv[0]
        if(len(argv)>=3):
            ipaddr=argv[1]
            port=argv[2]
            if(len(argv)>=4):
                client_port=argv[3]
                if(len(argv)>=5):
                    sim_time=int(argv[4])
                
    if len(argv)!=0:
        if (argv[0] == "-h" or argv[0]=="--help"):
            usage()
            sys.exit(0)
    else:
        print("using Server defaults =>")

    print("ip",ipaddr,"and port:",port)
    print("sim_time",sim_time)
    
    start_time = round(time.time(), 3)
    
    # Ensure sensor.list exists (create if sensor-launcher.py already ran)
    sensor_list_path = "./sensor.list"
    if not os.path.exists(sensor_list_path):
        print(f"Warning: {sensor_list_path} not found. Creating empty file.")
        print("Make sure sensor-launcher.py runs first to populate it!")
        open(sensor_list_path, 'w').close()
    
    # Find Q-table file
    q_table_paths = [
        "./q_agent_trained.csv",
        "../launcher/q_agent_trained.csv", 
        "../PRTP/application/q_agent_trained.csv",
        "./PRTP/application/q_agent_trained.csv"
    ]
    
    q_table_arg = ""
    for path in q_table_paths:
        if os.path.exists(path):
            q_table_arg = f"-q{path}"
            print(f"Found Q-table at: {path}")
            break
    
    if not q_table_arg:
        print("WARNING: q_agent_trained.csv not found in any expected location!")
        print("Searched:", q_table_paths)
        print("Q-learning will use untrained values.")
    
    # Build command
    cmd = [
        "../PRTP/application/PRTP_server", 
        f"-i{ipaddr}", 
        f"-p{port}", 
        f"-s{client_port}",
        f"-l{sensor_list_path}",
        f"-c{client_config}"
    ]
    
    if q_table_arg:
        cmd.append(q_table_arg)
    
    print(f"Starting PRTP_server with command: {' '.join(cmd)}")
    
    # Start IoT Server
    try:
        spawns.append(subprocess.Popen(cmd, shell=False))
        print(f"Server started with PID: {spawns[0].pid}")
    except Exception as e:
        print(f"ERROR starting server: {e}")
        sys.exit(1)
    
    time.sleep(sim_time)
    print("killing processes after", (time.time()-start_time))
    for s in spawns:
        try:
            os.kill(s.pid, signal.SIGINT)
        except:
            pass

if __name__ == "__main__":
    signal.signal(signal.SIGINT, signal_handler)
    main(sys.argv[1:])