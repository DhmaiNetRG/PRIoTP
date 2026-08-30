"""Handshake tracker module."""
import csv
import threading
from dataclasses import dataclass, field
from pathlib import Path
from typing import Optional

@dataclass
class HandshakeRecord:
    session_id: int
    start_ts: float
    response_ts: Optional[float] = None
    ack_ts: Optional[float] = None
    success_ts: Optional[float] = None
    failure_ts: Optional[float] = None
    failure_reason: Optional[str] = None
    retries: int = 0
    timeouts: int = 0
    latency_ms: Optional[float] = None
    state: str = 'pending'

class HandshakeTracker:
    def __init__(self):
        self._lock = threading.Lock()
        self.records: dict[int, HandshakeRecord] = {}

    def on_event(self, event: dict):
        event_type = event.get('event')
        session_id = event.get('session_id')
        ts = event.get('timestamp')
        if not event_type or session_id is None or ts is None:
            return

        if event_type == 'HandshakeStart':
            self.on_handshake_start(session_id, ts)
        elif event_type == 'HandshakeResponse':
            self.on_handshake_response(session_id, ts)
        elif event_type == 'HandshakeAck':
            self.on_handshake_ack(session_id, ts)
        elif event_type == 'HandshakeSuccess':
            self.on_handshake_success(session_id, ts, event.get('latency_ms'))
        elif event_type == 'HandshakeTimeout':
            self.on_handshake_timeout(session_id, ts, event.get('attempt', 1))
        elif event_type == 'HandshakeRetry':
            self.on_handshake_retry(session_id, ts, event.get('attempt', 1))
        elif event_type == 'HandshakeFailure':
            self.on_handshake_failure(session_id, ts, event.get('reason'))

    def on_handshake_start(self, session_id: int, ts: float):
        with self._lock:
            if session_id not in self.records:
                self.records[session_id] = HandshakeRecord(session_id=session_id, start_ts=ts)

    def on_handshake_response(self, session_id: int, ts: float):
        with self._lock:
            if session_id in self.records:
                self.records[session_id].response_ts = ts

    def on_handshake_ack(self, session_id: int, ts: float):
        with self._lock:
            if session_id in self.records:
                self.records[session_id].ack_ts = ts

    def on_handshake_success(self, session_id: int, ts: float, latency_ms: Optional[float]):
        with self._lock:
            if session_id in self.records:
                self.records[session_id].success_ts = ts
                self.records[session_id].latency_ms = latency_ms
                self.records[session_id].state = 'success'

    def on_handshake_timeout(self, session_id: int, ts: float, attempt: int):
        with self._lock:
            if session_id in self.records:
                self.records[session_id].timeouts += 1

    def on_handshake_retry(self, session_id: int, ts: float, attempt: int):
        with self._lock:
            if session_id in self.records:
                self.records[session_id].retries += 1

    def on_handshake_failure(self, session_id: int, ts: float, reason: Optional[str]):
        with self._lock:
            if session_id in self.records:
                self.records[session_id].failure_ts = ts
                self.records[session_id].failure_reason = reason
                self.records[session_id].state = 'failed'

    def get_summary(self) -> dict:
        with self._lock:
            total = len(self.records)
            success = sum(1 for r in self.records.values() if r.state == 'success')
            failed = sum(1 for r in self.records.values() if r.state == 'failed')
            pending = total - success - failed
            latencies = [r.latency_ms for r in self.records.values() if r.latency_ms is not None]
            avg_latency = sum(latencies) / len(latencies) if latencies else 0.0
            success_rate = (success / total) * 100 if total > 0 else 0.0

            return {
                'total': total,
                'success': success,
                'failed': failed,
                'pending': pending,
                'avg_latency_ms': avg_latency,
                'success_rate': success_rate
            }

    def export_csv(self, path: str = 'exports/handshake_metrics.csv'):
        out_path = Path(path)
        out_path.parent.mkdir(parents=True, exist_ok=True)
        
        with self._lock:
            records = list(self.records.values())

        with open(out_path, 'w', newline='') as f:
            writer = csv.writer(f)
            writer.writerow(['timestamp', 'session_id', 'event', 'latency_ms'])
            
            for r in records:
                writer.writerow([r.start_ts, r.session_id, 'HandshakeStart', ''])
                if r.response_ts:
                    writer.writerow([r.response_ts, r.session_id, 'HandshakeResponse', ''])
                if r.ack_ts:
                    writer.writerow([r.ack_ts, r.session_id, 'HandshakeAck', ''])
                if r.success_ts:
                    writer.writerow([r.success_ts, r.session_id, 'HandshakeSuccess', r.latency_ms or ''])
                if r.failure_ts:
                    writer.writerow([r.failure_ts, r.session_id, 'HandshakeFailure', ''])
