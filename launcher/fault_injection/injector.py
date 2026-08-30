"""Fault injector module."""
import socket
import threading
import time
import random
import struct
from dataclasses import dataclass, field
from typing import Optional

@dataclass
class FaultConfig:
    loss_pct: float = 0.0        # 0.0 - 1.0
    corrupt_pct: float = 0.0     # 0.0 - 1.0 probability of byte flip
    delay_ms: float = 0.0        # added delay in ms
    reorder_pct: float = 0.0     # probability of buffering for reorder
    replay_pct: float = 0.0      # probability of replaying a previous packet
    invalid_session_id: bool = False  # replace session ID with random value
    malformed_header: bool = False    # zero out first 4 bytes

class FaultInjector:
    def __init__(self, config: FaultConfig, listen_port: int, forward_host: str, forward_port: int):
        self.config = config
        self.listen_port = listen_port
        self.forward_host = forward_host
        self.forward_port = forward_port
        self.running = False
        self.thread: Optional[threading.Thread] = None
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.sock.bind(('0.0.0.0', listen_port))
        
        self.stats = {
            'packets_received': 0,
            'packets_forwarded': 0,
            'packets_dropped': 0,
            'packets_delayed': 0,
            'packets_corrupted': 0
        }
        
        self.replay_buffer = []

    def start(self) -> None:
        if self.running:
            return
        self.running = True
        self.thread = threading.Thread(target=self._proxy_loop, daemon=True)
        self.thread.start()

    def stop(self) -> None:
        self.running = False
        if self.sock:
            try:
                self.sock.close()
            except:
                pass
        if self.thread:
            self.thread.join(timeout=1.0)

    def get_stats(self) -> dict:
        return self.stats

    def _proxy_loop(self):
        # We also need a socket to send to the server
        forward_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        
        while self.running:
            try:
                data, addr = self.sock.recvfrom(65535)
                if not data:
                    continue
                self.stats['packets_received'] += 1
                
                modified_data = self._apply_faults(data)
                
                if modified_data is None:
                    self.stats['packets_dropped'] += 1
                    continue
                    
                if self.config.delay_ms > 0:
                    self.stats['packets_delayed'] += 1
                    time.sleep(self.config.delay_ms / 1000.0)
                
                forward_sock.sendto(modified_data, (self.forward_host, self.forward_port))
                self.stats['packets_forwarded'] += 1
                
            except Exception as e:
                if self.running:
                    print(f"Proxy error: {e}")

    def _apply_faults(self, data: bytes) -> Optional[bytes]:
        if random.random() < self.config.loss_pct:
            return None
            
        data_mut = bytearray(data)
        
        if random.random() < self.config.corrupt_pct:
            if len(data_mut) > 0:
                idx = random.randint(0, len(data_mut) - 1)
                data_mut[idx] ^= 0xFF
                self.stats['packets_corrupted'] += 1

        if self.config.invalid_session_id and len(data_mut) >= 4:
            # Assuming session id is at some offset, for now just mess up first few bytes or specific offset
            # Let's say offset 1-5 is session ID
            pass
            
        if self.config.malformed_header and len(data_mut) >= 4:
            data_mut[0:4] = b'\x00\x00\x00\x00'
            
        return bytes(data_mut)
