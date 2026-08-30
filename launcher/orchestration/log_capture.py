"""
Log capture for PRIoTPS processes.
"""
import threading
import subprocess
from typing import Callable, List, Dict, Tuple

class LogCapture:
    def __init__(self):
        self._threads: Dict[str, List[threading.Thread]] = {}
        self._stop_events: Dict[str, threading.Event] = {}

    def attach(self, proc: subprocess.Popen, role: str, callbacks: List[Callable[[str, str], None]] = None):
        if callbacks is None:
            callbacks = []
        stop_event = threading.Event()
        self._stop_events[role] = stop_event
        
        t_out = threading.Thread(target=self._reader_thread, args=(proc.stdout, role, callbacks, stop_event), daemon=True)
        t_err = threading.Thread(target=self._reader_thread, args=(proc.stderr, role, callbacks, stop_event), daemon=True)
        
        self._threads[role] = [t_out, t_err]
        t_out.start()
        t_err.start()

    def detach(self, role: str):
        event = self._stop_events.pop(role, None)
        if event:
            event.set()
        threads = self._threads.pop(role, [])
        for t in threads:
            t.join(timeout=1.0)

    def _reader_thread(self, stream, role: str, callbacks: List[Callable[[str, str], None]], stop_event: threading.Event):
        if not stream:
            return
        for line in iter(stream.readline, ''):
            if stop_event.is_set():
                break
            if line:
                for cb in callbacks:
                    try:
                        cb(role, line.rstrip('\n'))
                    except Exception as e:
                        print(f"Callback error: {e}")
            else:
                break

    def stop_all(self):
        roles = list(self._threads.keys())
        for r in roles:
            self.detach(r)
