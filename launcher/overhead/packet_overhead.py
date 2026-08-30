"""Packet overhead analysis."""
import csv
from dataclasses import dataclass
from pathlib import Path

# Constants matching the C implementation
SEH_DATA_OVERHEAD = 13   # 1 flag + 8 nonce + 4 session_id
ASCON_TAG_SIZE = 16
PRIOTPS_OVERHEAD = SEH_DATA_OVERHEAD + ASCON_TAG_SIZE  # 29 bytes

# BSON document overhead for a minimal PRIoTP message (empirically: ~32 bytes)
BSON_OVERHEAD = 32

@dataclass
class OverheadRecord:
    payload_size: int
    priotp_total: int      # BSON_OVERHEAD + payload_size
    priotps_total: int     # BSON_OVERHEAD + payload_size + PRIOTPS_OVERHEAD
    seh_size: int          # SEH_DATA_OVERHEAD = 13
    nonce_size: int        # 8 (part of SEH)
    tag_size: int          # ASCON_TAG_SIZE = 16
    expansion_pct: float   # (priotps_total - priotp_total) / priotp_total * 100

PAYLOAD_SIZES = [16, 32, 64, 128, 256, 512, 1024, 2048, 4096]

def compute_overhead(payload_sizes: list[int] = None) -> list[OverheadRecord]:
    if payload_sizes is None:
        payload_sizes = PAYLOAD_SIZES
    
    records = []
    for p in payload_sizes:
        priotp_total = BSON_OVERHEAD + p
        priotps_total = priotp_total + PRIOTPS_OVERHEAD
        expansion = ((priotps_total - priotp_total) / priotp_total) * 100.0
        
        records.append(OverheadRecord(
            payload_size=p,
            priotp_total=priotp_total,
            priotps_total=priotps_total,
            seh_size=SEH_DATA_OVERHEAD,
            nonce_size=8,
            tag_size=ASCON_TAG_SIZE,
            expansion_pct=expansion
        ))
    return records

def export_csv(records: list[OverheadRecord], path: str = 'exports/packet_overhead.csv') -> None:
    out_path = Path(path)
    out_path.parent.mkdir(parents=True, exist_ok=True)
    
    with open(out_path, 'w', newline='') as f:
        writer = csv.writer(f)
        writer.writerow([
            'payload_size', 'priotp_total', 'priotps_total', 
            'seh_size', 'nonce_size', 'tag_size', 'expansion_pct'
        ])
        for r in records:
            writer.writerow([
                r.payload_size, r.priotp_total, r.priotps_total,
                r.seh_size, r.nonce_size, r.tag_size, f"{r.expansion_pct:.2f}"
            ])

def run_overhead_analysis(output_path: str = 'exports/packet_overhead.csv') -> bool:
    try:
        records = compute_overhead()
        export_csv(records, output_path)
        return True
    except Exception as e:
        print(f"Error computing overhead: {e}")
        return False
