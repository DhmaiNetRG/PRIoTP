"""
Telemetry JSONL collector for PRIoTPS runtime events.
"""
import json
import threading
import time
from pathlib import Path
from typing import Callable, Dict, List

class TelemetryCollector:
    def __init__(self, telemetry_path: str, poll_interval: float = 0.1):
        self.telemetry_path = Path(telemetry_path)
        self.poll_interval = poll_interval
        self._thread = None
        self._stop_event = threading.Event()
        self._handlers: Dict[str, List[Callable[[dict], None]]] = {}

    def start(self):
        self._stop_event.clear()
        self._thread = threading.Thread(target=self._tail_loop, daemon=True)
        self._thread.start()

    def stop(self):
        self._stop_event.set()
        if self._thread:
            self._thread.join()

    def register_handler(self, event_type: str, handler: Callable[[dict], None]):
        if event_type not in self._handlers:
            self._handlers[event_type] = []
        self._handlers[event_type].append(handler)

    def _tail_loop(self):
        start_wait = time.time()
        while not self.telemetry_path.exists():
            if time.time() - start_wait > 30:
                print(f"Telemetry collector: timed out waiting for {self.telemetry_path}")
                return
            time.sleep(1)
            if self._stop_event.is_set():
                return

        with self.telemetry_path.open('r', encoding='utf-8') as f:
            f.seek(0, 2)
            while not self._stop_event.is_set():
                line = f.readline()
                if not line:
                    time.sleep(self.poll_interval)
                    continue
                try:
                    event = json.loads(line)
                    evt_type = event.get('type')
                    if evt_type in self._handlers:
                        for handler in self._handlers[evt_type]:
                            handler(event)
                    if '*' in self._handlers:
                        for handler in self._handlers['*']:
                            handler(event)
                except json.JSONDecodeError:
                    print(f"Warning: JSON Decode Error in line: {line.strip()}")
