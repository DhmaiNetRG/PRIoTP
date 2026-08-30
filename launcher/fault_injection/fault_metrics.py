"""Fault metrics collector."""
import csv
from dataclasses import dataclass
from pathlib import Path

@dataclass
class FaultRunResult:
    fault_type: str
    intensity: float
    packets_injected: int
    packets_delivered: int
    delivery_ratio: float
    handshakes_attempted: int
    handshakes_succeeded: int
    handshake_success_rate: float
    decrypt_attempts: int
    decrypt_successes: int
    decrypt_success_rate: float

class FaultMetricsCollector:
    def __init__(self):
        self.runs: list[FaultRunResult] = []

    def record_run(self, result: FaultRunResult):
        self.runs.append(result)

    def export_csv(self, path: str = 'exports/fault_injection_metrics.csv'):
        out_path = Path(path)
        out_path.parent.mkdir(parents=True, exist_ok=True)
        
        with open(out_path, 'w', newline='') as f:
            writer = csv.writer(f)
            writer.writerow([
                'fault_type', 'intensity', 'delivery_ratio', 
                'handshake_success_rate', 'decrypt_success_rate'
            ])
            for r in self.runs:
                writer.writerow([
                    r.fault_type, r.intensity, r.delivery_ratio,
                    r.handshake_success_rate, r.decrypt_success_rate
                ])
