"""
Main CLI entry point for PRIoTPS launcher.
"""
import argparse
import sys
from pathlib import Path
from typing import Optional

__version__ = '1.0.0'

def handle_build(args):
    from launcher.orchestration import process_manager
    print(f"Building target '{args.target}' with {args.jobs} jobs...")
    rc = process_manager.build(jobs=args.jobs, target=args.target)
    sys.exit(rc)

def handle_rebuild(args):
    from launcher.orchestration import process_manager
    print(f"Rebuilding with {args.jobs} jobs...")
    rc = process_manager.rebuild(jobs=args.jobs)
    sys.exit(rc)

def handle_start_broker(args):
    from launcher.orchestration import process_manager
    print("Starting broker...")
    process_manager.start_broker(
        args.host, args.sensor_port, args.client_port,
        args.sensor_list, args.client_config, args.q_table, args.telemetry
    )

def handle_stop_broker(args):
    from launcher.orchestration import process_manager
    print("Stopping broker...")
    process_manager.stop_broker()

def handle_start_client(args):
    from launcher.orchestration import process_manager
    print("Starting client...")
    process_manager.start_client(args.host, args.port, args.sensor_ids)

def handle_stop_client(args):
    # Example logic, assumes a way to specify client ID or stopping all clients
    print("Not fully implemented: stopping specific client")

def handle_start_sensor(args):
    from launcher.orchestration import process_manager
    print(f"Starting sensor {args.id}...")
    process_manager.start_sensor(args.type, args.host, args.port, args.id)

def handle_stop_sensor(args):
    from launcher.orchestration import process_manager
    # Assuming args.id is given, but adding it to argparser below is better.
    print("Stopping sensor...")

def handle_stop_all(args):
    from launcher.orchestration import process_manager
    print("Stopping all processes...")
    process_manager.stop_all()

def handle_status(args):
    from launcher.orchestration import process_manager
    status = process_manager.get_status()
    import pprint
    pprint.pprint(status)

def main():
    parser = argparse.ArgumentParser(description="PRIoTPS Launcher")
    subparsers = parser.add_subparsers(dest="subcommand", required=True)

    # Build
    build_parser = subparsers.add_parser("build")
    build_parser.add_argument("--jobs", type=int, default=4)
    build_parser.add_argument("--target", type=str, default="all", choices=["all", "benchmarks"])
    build_parser.set_defaults(func=handle_build)

    # Rebuild
    rebuild_parser = subparsers.add_parser("rebuild")
    rebuild_parser.add_argument("--jobs", type=int, default=4)
    rebuild_parser.set_defaults(func=handle_rebuild)

    # Start broker
    start_broker_parser = subparsers.add_parser("start-broker")
    start_broker_parser.add_argument("--host", default="localhost")
    start_broker_parser.add_argument("--sensor-port", type=int, default=5000)
    start_broker_parser.add_argument("--client-port", type=int, default=5001)
    start_broker_parser.add_argument("--sensor-list", default="./sensor.list")
    start_broker_parser.add_argument("--client-config", default="./PRTP/conf/test.conf")
    start_broker_parser.add_argument("--q-table", default="./launcher/q_agent_trained.csv")
    start_broker_parser.add_argument("--telemetry", default="./exports/priotps_telemetry.jsonl")
    start_broker_parser.set_defaults(func=handle_start_broker)

    # Stop broker
    stop_broker_parser = subparsers.add_parser("stop-broker")
    stop_broker_parser.set_defaults(func=handle_stop_broker)

    # Start client
    start_client_parser = subparsers.add_parser("start-client")
    start_client_parser.add_argument("--host", required=True)
    start_client_parser.add_argument("--port", type=int, default=5001)
    start_client_parser.add_argument("--sensor-ids", nargs="+")
    start_client_parser.set_defaults(func=handle_start_client)

    # Stop client
    stop_client_parser = subparsers.add_parser("stop-client")
    stop_client_parser.set_defaults(func=handle_stop_client)

    # Start sensor
    start_sensor_parser = subparsers.add_parser("start-sensor")
    start_sensor_parser.add_argument("--type", required=True)
    start_sensor_parser.add_argument("--host", required=True)
    start_sensor_parser.add_argument("--port", type=int, required=True)
    start_sensor_parser.add_argument("--id", required=True)
    start_sensor_parser.set_defaults(func=handle_start_sensor)

    # Stop sensor
    stop_sensor_parser = subparsers.add_parser("stop-sensor")
    stop_sensor_parser.set_defaults(func=handle_stop_sensor)

    # Stop all
    stop_all_parser = subparsers.add_parser("stop-all")
    stop_all_parser.set_defaults(func=handle_stop_all)

    # Status
    status_parser = subparsers.add_parser("status")
    status_parser.set_defaults(func=handle_status)

    # Benchmark
    benchmark_parser = subparsers.add_parser("benchmark")
    benchmark_parser.add_argument("--type", choices=["ascon", "handshake", "overhead", "all"], required=True)
    benchmark_parser.add_argument("--output-dir", default="./exports")

    # Run experiment
    run_experiment_parser = subparsers.add_parser("run-experiment")
    run_experiment_parser.add_argument("--clients", type=str, required=True)
    run_experiment_parser.add_argument("--payloads", type=str, required=True)
    run_experiment_parser.add_argument("--duration", type=int, required=True)
    run_experiment_parser.add_argument("--seed", type=int)

    # Run scenario
    run_scenario_parser = subparsers.add_parser("run-scenario")
    run_scenario_parser.add_argument("--profile", choices=["small", "medium", "large", "stress"], required=True)
    run_scenario_parser.add_argument("--seed", type=int)

    # Inject fault
    inject_fault_parser = subparsers.add_parser("inject-fault")
    inject_fault_parser.add_argument("--type", choices=["loss", "corrupt", "delay", "reorder", "replay", "all"], required=True)
    inject_fault_parser.add_argument("--intensity", type=float, required=True)
    inject_fault_parser.add_argument("--listen-port", type=int, required=True)
    inject_fault_parser.add_argument("--forward-host", required=True)
    inject_fault_parser.add_argument("--forward-port", type=int, required=True)

    # Collect KPIs
    collect_kpis_parser = subparsers.add_parser("collect-kpis")

    args = parser.parse_args()

    if hasattr(args, 'func'):
        args.func(args)
    else:
        print(f"Command '{args.subcommand}' parsed but not fully implemented.")

if __name__ == "__main__":
    main()
