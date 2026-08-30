"""
Exporters for saving PRIoTPS runtime telemetry to JSON and CSV.
"""
import json
import csv
import threading
import time
from pathlib import Path
from dataclasses import asdict
from .runtime_state import StateStore, RuntimeState

def export_json(state: RuntimeState, path: str) -> None:
    p = Path(path)
    p.parent.mkdir(parents=True, exist_ok=True)
    with p.open('w', encoding='utf-8') as f:
        json.dump(asdict(state), f, indent=4)

def export_csv(state: RuntimeState, path: str) -> None:
    p = Path(path)
    p.parent.mkdir(parents=True, exist_ok=True)
    write_header = not p.exists()
    
    with p.open('a', newline='', encoding='utf-8') as f:
        data = asdict(state)
        data['timestamp'] = time.time()
        writer = csv.DictWriter(f, fieldnames=data.keys())
        if write_header:
            writer.writeheader()
        writer.writerow(data)

class StreamExporter:
    def __init__(self, store: StateStore, output_dir: str, interval_sec: float = 5.0):
        self.store = store
        self.output_dir = Path(output_dir)
        self.interval_sec = interval_sec
        self._thread = None
        self._stop_event = threading.Event()

    def start(self):
        self._stop_event.clear()
        self._thread = threading.Thread(target=self._export_loop, daemon=True)
        self._thread.start()

    def stop(self):
        self._stop_event.set()
        if self._thread:
            self._thread.join()

    def _export_loop(self):
        self.output_dir.mkdir(parents=True, exist_ok=True)
        json_path = self.output_dir / "telemetry_snapshot.json"
        csv_path = self.output_dir / "telemetry_stream.csv"
        while not self._stop_event.is_set():
            state = self.store.snapshot()
            try:
                export_json(state, str(json_path))
                export_csv(state, str(csv_path))
            except Exception as e:
                print(f"Exporter error: {e}")
            self._stop_event.wait(self.interval_sec)
