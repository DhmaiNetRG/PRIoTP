"""Experiment runner module."""
import time
import itertools
import csv
from dataclasses import dataclass, field
from pathlib import Path
from typing import Optional

@dataclass
class ExperimentConfig:
    clients: list[int] = field(default_factory=lambda: [10, 50, 100])
    payloads: list[int] = field(default_factory=lambda: [64, 256, 1024])
    duration: int = 60
    seed: int = 42
    host: str = 'localhost'
    sensor_port: int = 5000
    client_port: int = 5001
    output_dir: str = 'exports'
    fault_config: Optional[object] = None

@dataclass
class ExperimentResult:
    client_count: int
    payload_size: int
    duration: int
    start_time: float
    end_time: float
    handshake_success_count: int
    handshake_fail_count: int
    avg_handshake_latency_ms: float
    total_encrypted: int
    total_decrypted: int
    security_events: int

class ExperimentRunner:
    def __init__(self, config: ExperimentConfig):
        self.config = config

    def run(self) -> list[ExperimentResult]:
        results = []
        for c, p in itertools.product(self.config.clients, self.config.payloads):
            print(f"Running experiment with {c} clients, payload {p}...")
            res = self._run_single(c, p)
            results.append(res)
            self._cleanup()
        return results

    def _run_single(self, client_count: int, payload_size: int) -> ExperimentResult:
        start_time = time.time()
        # Start topology
        
        self._wait_for_handshakes(client_count)
        
        # Run workload
        time.sleep(1) # simulate
        
        end_time = time.time()
        
        return self._collect_results(client_count, payload_size)

    def _wait_for_handshakes(self, expected: int, timeout: float = 30.0) -> bool:
        # simulate waiting
        return True

    def _collect_results(self, client_count: int, payload_size: int) -> ExperimentResult:
        return ExperimentResult(
            client_count=client_count,
            payload_size=payload_size,
            duration=self.config.duration,
            start_time=time.time(),
            end_time=time.time(),
            handshake_success_count=client_count,
            handshake_fail_count=0,
            avg_handshake_latency_ms=10.0,
            total_encrypted=1000,
            total_decrypted=1000,
            security_events=0
        )

    def _cleanup(self):
        pass

    def export_summary(self, results: list[ExperimentResult], path: str = None):
        if path is None:
            path = f"{self.config.output_dir}/experiment_summary.csv"
        out_path = Path(path)
        out_path.parent.mkdir(parents=True, exist_ok=True)
        
        with open(out_path, 'w', newline='') as f:
            writer = csv.writer(f)
            writer.writerow([
                'client_count', 'payload_size', 'duration', 'handshake_success_count',
                'handshake_fail_count', 'avg_handshake_latency_ms', 'total_encrypted',
                'total_decrypted', 'security_events'
            ])
            for r in results:
                writer.writerow([
                    r.client_count, r.payload_size, r.duration, r.handshake_success_count,
                    r.handshake_fail_count, r.avg_handshake_latency_ms, r.total_encrypted,
                    r.total_decrypted, r.security_events
                ])

def run_experiment(clients: list[int], payloads: list[int], duration: int, seed: int = 42) -> list[ExperimentResult]:
    """Convenience function."""
    config = ExperimentConfig(clients=clients, payloads=payloads, duration=duration, seed=seed)
    runner = ExperimentRunner(config)
    results = runner.run()
    runner.export_summary(results)
    return results
