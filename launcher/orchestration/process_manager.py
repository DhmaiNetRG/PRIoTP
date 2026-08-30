"""
Process manager for PRIoTPS launcher.
"""
import subprocess
import threading
import signal
import time
from pathlib import Path
from typing import Dict, List, Optional

class ProcessManager:
    def __init__(self):
        self._processes: Dict[str, subprocess.Popen] = {}
        self._lock = threading.Lock()

    def _resolve_binary(self, name: str) -> str:
        search_paths = [
            Path('.') / 'PRTP' / 'application' / name,
            Path('..') / 'PRTP' / 'application' / name,
            Path(name)
        ]
        for p in search_paths:
            if p.exists() or p.is_file():
                return str(p)
        return name

    def start_broker(self, host: str, sensor_port: int, client_port: int, sensor_list: str, client_config: str, q_table: str, telemetry_path: str) -> subprocess.Popen:
        with self._lock:
            if "broker" in self._processes:
                raise RuntimeError("Broker is already running.")
            bin_path = self._resolve_binary('PRTP_server')
            # PRTP_server uses POSIX getopt single-letter flags: -i -p -s -l -c -q -t
            cmd = [bin_path, f'-i{host}', f'-p{sensor_port}', f'-s{client_port}',
                   f'-l{sensor_list}', f'-c{client_config}']
            if q_table:
                cmd.append(f'-q{q_table}')
            if telemetry_path:
                cmd.append(f'-t{telemetry_path}')
            proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
            self._processes["broker"] = proc
            return proc


    def stop_broker(self) -> None:
        self._stop_process("broker")

    def start_client(self, host: str, port: int, sensor_ids: List[str], client_id: str = None) -> subprocess.Popen:
        with self._lock:
            cid = client_id or f"client_{len([k for k in self._processes if k.startswith('client_')])}"
            if cid in self._processes:
                raise RuntimeError(f"Client {cid} already running.")
            bin_path = self._resolve_binary('PRTP_client')
            cmd = [bin_path, '--host', host, '--port', str(port)]
            if sensor_ids:
                cmd.extend(['--sensor-ids', ",".join(sensor_ids)])
            proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
            self._processes[cid] = proc
            return proc

    def stop_client(self, client_id: str) -> None:
        self._stop_process(client_id)

    def start_sensor(self, sensor_type: str, host: str, port: int, sensor_id: str) -> subprocess.Popen:
        with self._lock:
            role = f"sensor_{sensor_id}"
            if role in self._processes:
                raise RuntimeError(f"Sensor {role} already running.")
            bin_path = self._resolve_binary('sensor.py')
            cmd = ['python', bin_path, '--type', sensor_type, '--host', host, '--port', str(port), '--id', sensor_id]
            proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
            self._processes[role] = proc
            return proc

    def stop_sensor(self, sensor_id: str) -> None:
        self._stop_process(f"sensor_{sensor_id}")

    def stop_all(self) -> None:
        with self._lock:
            keys = list(self._processes.keys())
        for k in keys:
            self._stop_process(k)

    def _stop_process(self, role: str):
        with self._lock:
            proc = self._processes.pop(role, None)
        if proc:
            if proc.poll() is None:
                try:
                    proc.send_signal(signal.SIGTERM)
                except Exception:
                    pass
                try:
                    proc.wait(timeout=3.0)
                except subprocess.TimeoutExpired:
                    proc.kill()
                    proc.wait()

    def get_status(self) -> Dict:
        with self._lock:
            status = {}
            for role, proc in self._processes.items():
                rc = proc.poll()
                status[role] = {
                    "pid": proc.pid,
                    "alive": rc is None,
                    "returncode": rc
                }
            return status

_manager = ProcessManager()

def start_broker(host, sensor_port, client_port, sensor_list, client_config, q_table, telemetry_path):
    return _manager.start_broker(host, sensor_port, client_port, sensor_list, client_config, q_table, telemetry_path)

def stop_broker():
    _manager.stop_broker()

def start_client(host, port, sensor_ids, client_id=None):
    return _manager.start_client(host, port, sensor_ids, client_id)

def stop_client(client_id):
    _manager.stop_client(client_id)

def start_sensor(sensor_type, host, port, sensor_id):
    return _manager.start_sensor(sensor_type, host, port, sensor_id)

def stop_sensor(sensor_id):
    _manager.stop_sensor(sensor_id)

def stop_all():
    _manager.stop_all()

def get_status():
    return _manager.get_status()

def build(jobs=4, target='all') -> int:
    search_paths = [Path('./PRTP'), Path('../PRTP'), Path('PRTP')]
    cwd = None
    for p in search_paths:
        if p.is_dir():
            cwd = p
            break
    if not cwd:
        print("Error: PRTP directory not found.")
        return 1
    return subprocess.run(['make', '-j', str(jobs), target], cwd=cwd).returncode

def rebuild(jobs=4) -> int:
    search_paths = [Path('./PRTP'), Path('../PRTP'), Path('PRTP')]
    cwd = None
    for p in search_paths:
        if p.is_dir():
            cwd = p
            break
    if not cwd:
        print("Error: PRTP directory not found.")
        return 1
    subprocess.run(['make', 'clean'], cwd=cwd)
    return subprocess.run(['make', '-j', str(jobs)], cwd=cwd).returncode
