"""
Health monitor for PRIoTPS orchestration.
"""
import threading
import time
from typing import Dict
from .process_manager import ProcessManager

class HealthMonitor:
    def __init__(self, process_manager: ProcessManager, restart_on_crash: bool = True, max_restarts: int = 3):
        self.manager = process_manager
        self.restart_on_crash = restart_on_crash
        self.max_restarts = max_restarts
        self._thread = None
        self._stop_event = threading.Event()
        self._restart_counts: Dict[str, int] = {}

    def start(self):
        self._stop_event.clear()
        self._thread = threading.Thread(target=self._monitor_loop, daemon=True)
        self._thread.start()

    def stop(self):
        self._stop_event.set()
        if self._thread:
            self._thread.join()

    def _monitor_loop(self):
        while not self._stop_event.is_set():
            time.sleep(2.0)
            status = self.manager.get_status()
            for role, stat in status.items():
                if not stat['alive']:
                    rc = stat['returncode']
                    if rc != 0:
                        print(f"[HealthMonitor] {role} exited unexpectedly with code {rc}.")
                        if self.restart_on_crash:
                            counts = self._restart_counts.get(role, 0)
                            if counts < self.max_restarts:
                                print(f"[HealthMonitor] Restarting {role} (Attempt {counts+1}/{self.max_restarts})...")
                                self._restart_counts[role] = counts + 1
                                # Real restart logic goes here but requires parsing original arguments
                            else:
                                print(f"[HealthMonitor] {role} max restarts reached.")
