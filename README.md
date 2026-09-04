# PRIoTPS: Secure Context-Aware Partial Reliable Application-Layer IoT Protocol

## Overview

PRIoTPS (Secure Partial-Reliable Internet of Things Protocol) is a novel application-layer protocol designed specifically for Internet of Things (IoT) environments with a strong emphasis on security and data integrity. The protocol addresses the fundamental trade-off between reliability, resource efficiency, and security in IoT systems. It introduces partial reliability mechanisms that allow applications to selectively trade off delivery guarantees for reduced computational overhead, while ensuring that the transmitted data remains secure and verifiable against tampering.

Unlike traditional protocols that enforce all-or-nothing delivery semantics, PRIoTPS enables fine-grained control over reliability and security requirements on a per-message basis. It is particularly suited for resource-constrained IoT devices and sensor networks where perfect reliability is not always necessary, but data integrity and proof of transmission are critical.

## Key Features

- **Security and Proof Mode**: Built-in mechanisms for tamper testing and verifying data integrity across the network.
- **Partial-Reliable Delivery Semantics**: Flexible delivery guarantees allowing applications to specify reliability requirements per message or flow.
- **Congestion Control Mechanisms**: Adaptive congestion management optimized for IoT environments with heterogeneous network conditions.
- **Efficient Message Encoding**: BSON-based serialization for compact and efficient message representation.
- **Flow Management**: Sophisticated active flow monitoring and management for multi-source data aggregation.
- **Fragment Buffering**: Intelligent packet fragmentation and reassembly for networks with limited MTU.
- **Scalable Architecture**: Designed to support client-server communication patterns securely.

## Architecture

The PRIoTPS implementation consists of several core modules:

- **Protocol Core**: Core protocol logic, security policies, and state management.
- **Message Encoding**: BSON-based secure message serialization and parsing.
- **Congestion Control**: Adaptive algorithms for network resource management.
- **Flow Management**: Active flow tracking and routing.
- **Client Module**: Client-side secure protocol implementation.
- **Application Layer**: Sample applications and testing harnesses.

## Building and Installation

### Prerequisites

- C compiler (gcc or compatible)
- GNU Autotools (autoconf, automake, libtool)
- POSIX-compliant operating system
- libm (mathematics library)

### Compilation Steps

1. Navigate to the project directory and run the configure script:

```bash
./configure
```

The configure script will check for available system features and dependencies.

2. Compile the project using make:

```bash
make
```

3. Optionally, run the included test suite:

```bash
make check
```

4. Install the compiled binaries and libraries:

```bash
sudo make install
```

For local installation, use `./configure --prefix=/path/to/install` during step 1.

5. To verify successful installation, run the installation checks:

```bash
make installcheck
```

6. To clean the build artifacts:

```bash
make clean
```

To remove all generated configuration files:

```bash
make distclean
```

## Usage

### Starting the Test Environment

The protocol can be tested using the included sensor launcher and server components.

#### Standard Mode

**Terminal 1 - PRIoTPS Server**
Start the sensor simulator and traffic generator:

```bash
python3 sensor-launcher.py localhost 5004 10 4 &
python3 STGen_server.py ../conf/test.conf localhost 5004 5005 10
```

**Terminal 2 - PRIoTPS Client**
Launch the PRIoTPS client:

```bash
./PRTP_client -l./client1_sensor_log -s127.0.0.1 -rtemp_1 -p5005 -A
```

#### Proof Mode (Security & Tamper Testing)

To enable proof mode and tamper testing on both the server and client, you need to set the appropriate environment variables. This ensures that the processes inherit the proof-mode environment.

**Terminal 1 — PRIoTPS Server**

```bash
export PRIOTPS_PROOF_MODE=1
export PRIOTPS_TAMPER_TEST=1

python3 sensor-launcher.py localhost 5004 10 4 &
python3 STGen_server.py ../conf/test.conf localhost 5004 5005 10
```

**Terminal 2 — PRIoTPS Client**

Similarly, set the environment variables for the client:

```bash
export PRIOTPS_PROOF_MODE=1
export PRIOTPS_TAMPER_TEST=1

./PRTP_client -l./client1_sensor_log -s127.0.0.1 -rtemp_1 -p5005 -A
```

Alternatively, you can run the client without exporting by passing the variables inline:

```bash
PRIOTPS_PROOF_MODE=1 PRIOTPS_TAMPER_TEST=1 \
./PRTP_client -l./client1_sensor_log -s127.0.0.1 -rtemp_1 -p5005 -A
```

### Configuration

The protocol can be configured via configuration files located in the `conf/` directory. Refer to sample configuration files for available parameters, tuning options, and security settings.

## Experimental Evaluation

The `docs/eval/` directory contains comprehensive documentation of the protocol's experimental evaluation, including:

- Performance benchmarks and comparative analysis
- Congestion control behavior under various network conditions
- Lessons learned and recommendations for deployment
- Detailed appendices with supplementary experimental data

## Directory Structure

```text
.
├── src/                    Core protocol implementation source code
├── application/            Client and server applications
├── tests/                  Test suite and validation scripts
├── conf/                   Configuration files for experimental setups
├── docs/                   Specification documents and evaluation reports
├── launcher/               Sensor simulation and test infrastructure
└── PRTP/                   Main protocol package
    ├── src/                Protocol source files
    ├── application/        Application binaries and client logs
    ├── tests/              Protocol test suite
    └── conf/               Configuration examples
```

## Research Contributions

This work presents:

- **Novel Protocol Design**: PRIoTPS introduces a secure, partially-reliable IoT protocol over UDP, balancing speed, reliability, and security for resource-constrained IoT devices.
- **Theoretical Insights**: The research provides new understanding of the trade-offs between reliability, security, and speed, offering solutions for lightweight, efficient communication in IoT.
- **Improved Performance**: PRIoTPS demonstrates low-latency, efficient communication with optional reliability features like timestamps and fragmentation flags, without the overhead of TCP.
- **IoT Impact**: PRIoTPS offers a scalable and secure solution for IoT networks, addressing the limitations of existing protocols and enabling efficient communication for devices like sensors and wearables.
- **Practical Application**: The research provides solutions for resource-constrained devices, optimizing battery life and processing power in real-time secure communication.

## License

See the `COPYING` file for license information.

## Contributing

Contributions are welcome. Please read `CONTRIBUTING.md` for development workflow, coding expectations, and pull request guidelines.
