"""
Runtime state data structures for PRIoTPS telemetry.
"""
from dataclasses import dataclass, field
import threading

@dataclass
class RuntimeState:
    active_clients: int = 0
    active_sessions: int = 0
    pending_handshakes: int = 0
    completed_handshakes: int = 0
    failed_handshakes: int = 0
    total_encryptions: int = 0
    total_decryptions: int = 0
    total_bytes_encrypted: int = 0
    total_bytes_decrypted: int = 0
    retransmissions: int = 0
    security_events: int = 0
    rl_uack_allow: int = 0
    rl_uack_drop: int = 0

class StateStore:
    def __init__(self):
        self._state = RuntimeState()
        self._lock = threading.Lock()

    def update(self, **kwargs):
        with self._lock:
            for k, v in kwargs.items():
                if hasattr(self._state, k):
                    setattr(self._state, k, v)
                else:
                    raise AttributeError(f"RuntimeState has no attribute {k}")

    def snapshot(self) -> RuntimeState:
        with self._lock:
            from dataclasses import replace
            return replace(self._state)

    def reset(self):
        with self._lock:
            self._state = RuntimeState()

state_store = StateStore()
