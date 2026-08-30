"""Ascon benchmark runner."""
import subprocess
import csv
import io
from pathlib import Path
from typing import Optional

BENCH_ASCON_PATHS = [
    './PRTP/application/bench_ascon',
    './PRTP/src/bench_ascon',
    '../PRTP/application/bench_ascon',
]

def find_bench_ascon() -> Optional[str]:
    """Find the bench_ascon binary."""
    for p in BENCH_ASCON_PATHS:
        if Path(p).exists() and Path(p).is_file():
            return p
    return None

def run_ascon_benchmark(output_path: str = 'exports/ascon_benchmark.csv') -> bool:
    """Run bench_ascon and write results to output_path. Returns True on success."""
    binary = find_bench_ascon()
    if not binary:
        print("Error: bench_ascon binary not found.")
        return False
        
    out_path = Path(output_path)
    out_path.parent.mkdir(parents=True, exist_ok=True)
    
    try:
        result = subprocess.run([binary], capture_output=True, text=True, check=True)
        with open(out_path, 'w') as f:
            f.write(result.stdout)
        return True
    except subprocess.CalledProcessError as e:
        print(f"Error running bench_ascon: {e}")
        return False
    except Exception as e:
        print(f"Unexpected error: {e}")
        return False
