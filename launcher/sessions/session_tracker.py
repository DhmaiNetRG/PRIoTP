"""Session tracker module."""
import csv
import threading
from dataclasses import dataclass, field
from pathlib import Path
from typing import Optional

@dataclass
class SessionRecord:
    session_id: int
    created_at: float
    last_activity: float
    expired_at: Optional[float] = None
    duration_s: float = 0.0
    msgs_encrypted: int = 0
    msgs_decrypted: int = 0
    bytes_tx: int = 0
    bytes_rx: int = 0
    state: str = 'active'

class SessionTracker:
    def __init__(self):
        self._lock = threading.Lock()
        self.records: dict[int, SessionRecord] = {}

    def on_event(self, event: dict):
        event_type = event.get('event')
        session_id = event.get('session_id')
        ts = event.get('timestamp')
        if not event_type or session_id is None or ts is None:
            return

        if event_type == 'SessionCreated':
            self.on_session_created(session_id, ts)
        elif event_type == 'Encrypt':
            self.on_encrypt(session_id, ts, event.get('bytes_count', 0))
        elif event_type == 'Decrypt':
            self.on_decrypt(session_id, ts, event.get('bytes_count', 0))
        elif event_type == 'SessionExpired':
            self.on_session_expired(session_id, ts, event.get('duration_s', 0.0))

    def on_session_created(self, session_id: int, ts: float):
        with self._lock:
            if session_id not in self.records:
                self.records[session_id] = SessionRecord(session_id=session_id, created_at=ts, last_activity=ts)

    def on_encrypt(self, session_id: int, ts: float, bytes_count: int):
        with self._lock:
            if session_id in self.records:
                self.records[session_id].last_activity = ts
                self.records[session_id].msgs_encrypted += 1
                self.records[session_id].bytes_tx += bytes_count

    def on_decrypt(self, session_id: int, ts: float, bytes_count: int):
        with self._lock:
            if session_id in self.records:
                self.records[session_id].last_activity = ts
                self.records[session_id].msgs_decrypted += 1
                self.records[session_id].bytes_rx += bytes_count

    def on_session_expired(self, session_id: int, ts: float, duration_s: float):
        with self._lock:
            if session_id in self.records:
                self.records[session_id].expired_at = ts
                self.records[session_id].duration_s = duration_s
                self.records[session_id].state = 'expired'

    def get_active_count(self) -> int:
        with self._lock:
            return sum(1 for r in self.records.values() if r.state == 'active')

    def get_summary(self) -> dict:
        with self._lock:
            total = len(self.records)
            active = sum(1 for r in self.records.values() if r.state == 'active')
            expired = total - active
            total_encrypted = sum(r.msgs_encrypted for r in self.records.values())
            total_decrypted = sum(r.msgs_decrypted for r in self.records.values())
            total_bytes_tx = sum(r.bytes_tx for r in self.records.values())
            total_bytes_rx = sum(r.bytes_rx for r in self.records.values())
            
            return {
                'total': total,
                'active': active,
                'expired': expired,
                'total_encrypted': total_encrypted,
                'total_decrypted': total_decrypted,
                'total_bytes_tx': total_bytes_tx,
                'total_bytes_rx': total_bytes_rx
            }

    def export_csv(self, path: str = 'exports/session_metrics.csv'):
        out_path = Path(path)
        out_path.parent.mkdir(parents=True, exist_ok=True)
        
        with self._lock:
            records = list(self.records.values())

        with open(out_path, 'w', newline='') as f:
            writer = csv.writer(f)
            writer.writerow(['session_id', 'created_at', 'duration_s', 'bytes_tx', 'bytes_rx', 'msgs_encrypted', 'msgs_decrypted', 'state'])
            
            for r in records:
                writer.writerow([
                    r.session_id, r.created_at, r.duration_s, r.bytes_tx, r.bytes_rx,
                    r.msgs_encrypted, r.msgs_decrypted, r.state
                ])
