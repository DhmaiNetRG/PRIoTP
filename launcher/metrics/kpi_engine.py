"""KPI engine module."""
import csv
from dataclasses import dataclass
from pathlib import Path
from typing import Optional
try:
    import psutil
except ImportError:
    psutil = None

@dataclass
class KPIRecord:
    category: str   # Security | Protocol | System
    metric: str
    value: float
    unit: str
    scenario: str = 'default'

class KPIEngine:
    def __init__(self, exports_dir: str = 'exports'):
        self.exports_dir = Path(exports_dir)

    def _read_csv(self, filename: str) -> list[dict]:
        filepath = self.exports_dir / filename
        if not filepath.exists():
            print(f"Warning: {filename} not found.")
            return []
        try:
            with open(filepath, 'r') as f:
                reader = csv.DictReader(f)
                return list(reader)
        except Exception as e:
            print(f"Error reading {filename}: {e}")
            return []

    def compute_security_kpis(self) -> list[KPIRecord]:
        kpis = []
        # Handshake metrics
        hs_data = self._read_csv('handshake_metrics.csv')
        if hs_data:
            latencies = [float(row['latency_ms']) for row in hs_data if row.get('event') == 'HandshakeSuccess' and row.get('latency_ms')]
            if latencies:
                kpis.append(KPIRecord('Security', 'Handshake Avg Latency', sum(latencies)/len(latencies), 'ms'))
                kpis.append(KPIRecord('Security', 'Handshake Min Latency', min(latencies), 'ms'))
                kpis.append(KPIRecord('Security', 'Handshake Max Latency', max(latencies), 'ms'))
            
            starts = len([row for row in hs_data if row.get('event') == 'HandshakeStart'])
            successes = len([row for row in hs_data if row.get('event') == 'HandshakeSuccess'])
            if starts > 0:
                kpis.append(KPIRecord('Security', 'Handshake Success Ratio', (successes / starts) * 100, '%'))
        return kpis

    def compute_protocol_kpis(self) -> list[KPIRecord]:
        kpis = []
        oh_data = self._read_csv('packet_overhead.csv')
        if oh_data:
            expansions = [float(row['expansion_pct']) for row in oh_data if row.get('expansion_pct')]
            if expansions:
                kpis.append(KPIRecord('Protocol', 'Avg Data Expansion', sum(expansions)/len(expansions), '%'))
                
            overhead_bytes = [float(row['priotps_total']) - float(row['priotp_total']) for row in oh_data if row.get('priotps_total') and row.get('priotp_total')]
            if overhead_bytes:
                kpis.append(KPIRecord('Protocol', 'Avg Packet Overhead', sum(overhead_bytes)/len(overhead_bytes), 'bytes'))
        return kpis

    def compute_system_kpis(self) -> list[KPIRecord]:
        kpis = []
        sess_data = self._read_csv('session_metrics.csv')
        if sess_data:
            total_tx = sum(float(row['bytes_tx']) for row in sess_data if row.get('bytes_tx'))
            durations = [float(row['duration_s']) for row in sess_data if row.get('duration_s') and float(row['duration_s']) > 0]
            if durations and total_tx > 0:
                avg_duration = sum(durations)/len(durations)
                throughput = total_tx / avg_duration if avg_duration > 0 else 0
                kpis.append(KPIRecord('System', 'Avg Throughput', throughput, 'bytes/s'))
                
        if psutil:
            kpis.append(KPIRecord('System', 'CPU Usage', psutil.cpu_percent(), '%'))
            mem = psutil.virtual_memory()
            kpis.append(KPIRecord('System', 'Memory Usage', mem.used / (1024*1024), 'MB'))
            
        return kpis

    def compute_all(self) -> list[KPIRecord]:
        kpis = []
        kpis.extend(self.compute_security_kpis())
        kpis.extend(self.compute_protocol_kpis())
        kpis.extend(self.compute_system_kpis())
        return kpis

    def export_csv(self, records: list[KPIRecord], path: str = 'exports/kpi_results.csv'):
        out_path = Path(path)
        out_path.parent.mkdir(parents=True, exist_ok=True)
        
        with open(out_path, 'w', newline='') as f:
            writer = csv.writer(f)
            writer.writerow(['category', 'metric', 'value', 'unit', 'scenario'])
            for r in records:
                writer.writerow([r.category, r.metric, f"{r.value:.2f}", r.unit, r.scenario])

def compute_and_export(exports_dir: str = 'exports') -> bool:
    """Convenience function. Returns True on success."""
    try:
        engine = KPIEngine(exports_dir)
        records = engine.compute_all()
        engine.export_csv(records, f"{exports_dir}/kpi_results.csv")
        return True
    except Exception as e:
        print(f"Error computing KPIs: {e}")
        return False
