"""Scenario profiles."""
from .generator import ScenarioConfig

PROFILES: dict[str, ScenarioConfig] = {
    'small': ScenarioConfig(
        sensor_count=4, subscriber_count=2, topic_count=4,
        payload_size=64, message_rate_hz=1.0, duration_sec=30, seed=42
    ),
    'medium': ScenarioConfig(
        sensor_count=20, subscriber_count=10, topic_count=20,
        payload_size=256, message_rate_hz=5.0, duration_sec=60, seed=42
    ),
    'large': ScenarioConfig(
        sensor_count=100, subscriber_count=50, topic_count=100,
        payload_size=512, message_rate_hz=10.0, duration_sec=120, seed=42
    ),
    'stress': ScenarioConfig(
        sensor_count=500, subscriber_count=250, topic_count=500,
        payload_size=1024, message_rate_hz=50.0, duration_sec=300, seed=42
    ),
}

def get_profile(name: str) -> ScenarioConfig:
    if name not in PROFILES:
        raise ValueError(f'Unknown profile: {name}. Available: {list(PROFILES)}')
    return PROFILES[name]
