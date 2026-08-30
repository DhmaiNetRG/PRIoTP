"""Security event collector module."""
import csv
import threading
from dataclasses import dataclass
from pathlib import Path

@dataclass
class SecurityEvent:
    timestamp: float
    event_type: str
    session_id: int
    detail: str

class SecurityEventCollector:
    def __init__(self):
        self._lock = threading.Lock()
        self.events: list[SecurityEvent] = []

    def on_event(self, event: dict):
        if event.get('event') == 'SecurityEvent':
            ts = event.get('timestamp', 0.0)
            evt_type = event.get('event_type', 'Unknown')
            session_id = event.get('session_id', 0)
            detail = event.get('detail', '')
            with self._lock:
                self.events.append(SecurityEvent(
                    timestamp=ts,
                    event_type=evt_type,
                    session_id=session_id,
                    detail=detail
                ))

    def get_events(self) -> list[SecurityEvent]:
        with self._lock:
            return list(self.events)

    def get_counts_by_type(self) -> dict[str, int]:
        counts = {}
        with self._lock:
            for evt in self.events:
                counts[evt.event_type] = counts.get(evt.event_type, 0) + 1
        return counts

    def export_csv(self, path: str = 'exports/security_events.csv'):
        out_path = Path(path)
        out_path.parent.mkdir(parents=True, exist_ok=True)
        
        with self._lock:
            events_copy = list(self.events)

        with open(out_path, 'w', newline='') as f:
            writer = csv.writer(f)
            writer.writerow(['timestamp', 'event_type', 'session_id', 'detail'])
            for e in events_copy:
                writer.writerow([e.timestamp, e.event_type, e.session_id, e.detail])
