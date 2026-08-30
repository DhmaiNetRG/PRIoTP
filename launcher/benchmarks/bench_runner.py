"""Benchmark runner module."""
from .ascon_bench import run_ascon_benchmark
from .handshake_bench import run_handshake_benchmark
import time
from pathlib import Path

def run_all_benchmarks(output_dir: str = 'exports') -> dict[str, bool]:
    """Run all benchmarks and return a dict of {name: success}."""
    results = {}
    results['ascon'] = run_ascon_benchmark(f"{output_dir}/ascon_benchmark.csv")
    results['handshake'] = run_handshake_benchmark(f"{output_dir}/handshake_benchmark.csv")
    return results

def run_benchmark(bench_type: str, output_dir: str = 'exports') -> bool:
    """Run a specific benchmark. bench_type: ascon | handshake | all"""
    if bench_type == 'ascon':
        return run_ascon_benchmark(f"{output_dir}/ascon_benchmark.csv")
    elif bench_type == 'handshake':
        return run_handshake_benchmark(f"{output_dir}/handshake_benchmark.csv")
    elif bench_type == 'all':
        res = run_all_benchmarks(output_dir)
        return all(res.values())
    else:
        print(f"Unknown benchmark type: {bench_type}")
        return False
