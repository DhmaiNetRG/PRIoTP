"""Handshake benchmark runner."""
import subprocess
import csv
import io
from pathlib import Path
from typing import Optional

BENCH_HANDSHAKE_PATHS = [
    './PRTP/application/bench_handshake',
    './PRTP/src/bench_handshake',
    '../PRTP/application/bench_handshake',
]

def find_bench_handshake() -> Optional[str]:
    """Find the bench_handshake binary."""
    for p in BENCH_HANDSHAKE_PATHS:
        if Path(p).exists() and Path(p).is_file():
            return p
    return None

def run_handshake_benchmark(output_path: str = 'exports/handshake_benchmark.csv') -> bool:
    """Run bench_handshake and write results to output_path. Returns True on success."""
    binary = find_bench_handshake()
    if not binary:
        print("Error: bench_handshake binary not found.")
        return False
        
    out_path = Path(output_path)
    out_path.parent.mkdir(parents=True, exist_ok=True)
    
    try:
        result = subprocess.run([binary], capture_output=True, text=True, check=True)
        with open(out_path, 'w') as f:
            f.write(result.stdout)
        return True
    except subprocess.CalledProcessError as e:
        print(f"Error running bench_handshake: {e}")
        return False
    except Exception as e:
        print(f"Unexpected error: {e}")
        return False
