"""Scenario generator."""
import random
import os
from dataclasses import dataclass
from pathlib import Path

@dataclass
class ScenarioConfig:
    sensor_count: int = 4
    subscriber_count: int = 2
    topic_count: int = 4
    payload_size: int = 64
    message_rate_hz: float = 1.0
    duration_sec: int = 30
    seed: int = 42
    host: str = 'localhost'
    sensor_port: int = 5000
    client_port: int = 5001

class ScenarioGenerator:
    SENSOR_TYPES = ['temp', 'device', 'gps', 'camera']

    def __init__(self, config: ScenarioConfig):
        self.config = config
        random.seed(config.seed)

    def generate_sensor_list(self, output_path: str = './sensor.list') -> list[tuple[str, str]]:
        sensors = []
        for i in range(self.config.sensor_count):
            stype = random.choice(self.SENSOR_TYPES)
            sid = f"sensor_{i:04d}"
            sensors.append((stype, sid))
        self.write_sensor_list(sensors, output_path)
        return sensors

    def write_sensor_list(self, sensors: list[tuple], path: str):
        out_path = Path(path)
        out_path.parent.mkdir(parents=True, exist_ok=True)
        with open(out_path, 'w') as f:
            for s in sensors:
                f.write(f"{s[0]},{s[1]}\n")

    def start(self, process_manager) -> None:
        # Placeholder for starting processes
        pass

    def stop(self, process_manager) -> None:
        # Placeholder for stopping processes
        pass
