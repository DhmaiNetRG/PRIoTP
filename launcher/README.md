# PRIoTPS Launcher — Research Platform Control Plane

The PRIoTPS Launcher provides a comprehensive Python control plane for benchmarking, analyzing, and automating experiments with the PRIoTPS protocol.

## Prerequisites
- Python 3.10+
- `psutil` (for system KPIs)
- Pre-built PRTP binaries (`bench_ascon`, `bench_handshake`) available in standard locations.

Install dependencies:
```bash
pip install -r requirements.txt
```

## Quick Start

You can run various commands via `priotps_launcher.py`. Use `--help` for all options.

```bash
# Run all benchmarks
python priotps_launcher.py bench all

# Compute overhead metrics
python priotps_launcher.py overhead

# Run experiment
python priotps_launcher.py experiment --clients 10,50 --payloads 64,256

# Compute KPIs
python priotps_launcher.py kpis
```

## Directory Structure

| Package | Role |
|---------|------|
| `benchmarks` | PRIoTPS benchmark harness for ASCON and Handshake |
| `experiments` | PRIoTPS experiment automation |
| `fault_injection` | PRIoTPS fault injection framework (UDP proxy) |
| `handshake` | PRIoTPS handshake reliability tracking |
| `metrics` | PRIoTPS KPI collection and computation |
| `overhead` | PRIoTPS packet overhead analysis |
| `scenario` | PRIoTPS traffic scenario generation |
| `security_events` | PRIoTPS security event collection |
| `sessions` | PRIoTPS session lifecycle tracking |

## Generated Outputs

Metrics and results are saved as CSV files in the `exports/` directory:

| File | Description |
|------|-------------|
| `ascon_benchmark.csv` | Output of the ASCON benchmark binary |
| `handshake_benchmark.csv` | Output of the Handshake benchmark binary |
| `handshake_metrics.csv` | Handshake tracker events and latencies |
| `session_metrics.csv` | Session lifecycle statistics |
| `security_events.csv` | Captured security-related protocol events |
| `packet_overhead.csv` | Analytical packet size/expansion metrics |
| `fault_injection_metrics.csv` | Proxy packet loss/delivery/fault metrics |
| `experiment_summary.csv` | Summary of completed automated experiments |
| `kpi_results.csv` | Aggregate Protocol, Security, and System KPIs |
