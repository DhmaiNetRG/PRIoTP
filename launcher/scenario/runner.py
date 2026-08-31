"""
Scenario runner — orchestrates a full named scenario lifecycle:
  1. Load the profile
  2. Generate a sensor list
  3. Start the PRTP_server broker (if binary is available)
  4. Simulate sensors / wait for duration
  5. Stop the broker
  6. Print a summary
"""
import sys
import time
import threading
from pathlib import Path

from .profiles import get_profile
from .generator import ScenarioGenerator, ScenarioConfig


class ScenarioRunner:
    """
    Runs a complete named scenario (small / medium / large / stress).

    If the PRTP_server binary is not available on this platform (e.g., Windows
    without WSL), the runner still generates the scenario configuration files
    and simulates the timing, so the rest of the pipeline (KPI collection, etc.)
    can be exercised.
    """

    def __init__(self, profile: str, seed: int = None):
        self.config: ScenarioConfig = get_profile(profile)
        if seed is not None:
            self.config.seed = seed
        self.profile_name = profile
        self._process_manager = None
        self._broker_proc = None

    # ------------------------------------------------------------------
    # Public API
    # ------------------------------------------------------------------

    def run(self) -> None:
        self._print_header()
        sensor_list_path = self._generate_files()
        broker_started = self._start_broker(sensor_list_path)
        self._wait(broker_started)
        self._stop_broker(broker_started)
        self._print_summary()

    # ------------------------------------------------------------------
    # Internal steps
    # ------------------------------------------------------------------

    def _print_header(self) -> None:
        c = self.config
        print(f'\n{"=" * 60}')
        print(f'  Scenario: {self.profile_name}')
        print(f'  Sensors      : {c.sensor_count}')
        print(f'  Subscribers  : {c.subscriber_count}')
        print(f'  Topics       : {c.topic_count}')
        print(f'  Payload size : {c.payload_size} B')
        print(f'  Message rate : {c.message_rate_hz} Hz')
        print(f'  Duration     : {c.duration_sec} s')
        print(f'  Seed         : {c.seed}')
        print(f'{"=" * 60}')

    def _generate_files(self) -> str:
        """Generate sensor.list and return its path."""
        generator = ScenarioGenerator(self.config)
        sensor_list_path = './exports/scenario_sensor.list'
        Path('./exports').mkdir(parents=True, exist_ok=True)
        sensors = generator.generate_sensor_list(sensor_list_path)
        print(f'  [scenario] Generated {len(sensors)} sensor entries -> {sensor_list_path}')
        return sensor_list_path

    def _start_broker(self, sensor_list_path: str) -> bool:
        """Try to start PRTP_server. Returns True if successfully started."""
        try:
            from launcher.orchestration import process_manager
            self._process_manager = process_manager
        except ImportError:
            print('  [scenario] WARNING: Could not import process_manager. '
                  'Running in simulation-only mode.')
            return False

        client_config = './PRTP/conf/test.conf'
        q_table = './launcher/q_agent_trained.csv'
        telemetry = './exports/priotps_telemetry.jsonl'

        # Check if binary actually exists
        pm_obj = process_manager._manager
        broker_binary = pm_obj._resolve_binary('PRTP_server')
        if not Path(broker_binary).is_file() and broker_binary == 'PRTP_server':
            # Binary not on PATH either → simulation mode
            _print_broker_warning()
            return False

        try:
            self._broker_proc = process_manager.start_broker(
                host=self.config.host,
                sensor_port=self.config.sensor_port,
                client_port=self.config.client_port,
                sensor_list=sensor_list_path,
                client_config=client_config,
                q_table=q_table,
                telemetry_path=telemetry,
            )
            time.sleep(0.5)  # allow broker to initialise
            rc = self._broker_proc.poll()
            if rc is not None:
                stderr = self._broker_proc.stderr.read() if self._broker_proc.stderr else ''
                print(f'  [scenario] Broker exited immediately (rc={rc}): {stderr.strip()}')
                _print_broker_warning()
                return False

            print(f'  [scenario] Broker started  (pid={self._broker_proc.pid})')
            return True

        except Exception as e:
            print(f'  [scenario] Could not start broker: {e}')
            _print_broker_warning()
            return False

    def _wait(self, broker_running: bool) -> None:
        duration = self.config.duration_sec
        if broker_running:
            print(f'  [scenario] Running for {duration} s …')
        else:
            print(f'  [scenario] Simulation mode — sleeping {duration} s …')

        # Progress bar
        bar_width = 40
        start = time.time()
        while True:
            elapsed = time.time() - start
            if elapsed >= duration:
                break
            pct = elapsed / duration
            filled = int(bar_width * pct)
            bar = '█' * filled + '░' * (bar_width - filled)
            sys.stdout.write(f'\r  [{bar}] {elapsed:.0f}/{duration}s')
            sys.stdout.flush()
            time.sleep(0.5)
        sys.stdout.write(f'\r  [{"█" * bar_width}] {duration}/{duration}s\n')
        sys.stdout.flush()

    def _stop_broker(self, broker_running: bool) -> None:
        if not broker_running or self._process_manager is None:
            return
        try:
            self._process_manager.stop_broker()
            print('  [scenario] Broker stopped.')
        except Exception as e:
            print(f'  [scenario] Error stopping broker: {e}')

    def _print_summary(self) -> None:
        print(f'\n  Scenario "{self.profile_name}" finished.')
        print('  Outputs written to exports/\n')


# ---------------------------------------------------------------------------
# Helper
# ---------------------------------------------------------------------------

def _print_broker_warning() -> None:
    print(
        '\n'
        '  [!] PRTP_server binary not found or failed to start.\n'
        '     Running in simulation-only mode (sensor list generated, timing simulated).\n'
        '     To run a full live scenario:\n'
        '       - On Linux/WSL:   cd PRTP && make && cd ..\n'
        '       - On Windows:     install MinGW or use WSL, then run:\n'
        '                         python priotps_launcher.py build\n'
    )
