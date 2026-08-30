# PRIoTPS

## Partially Reliable IoT Protocol Secure

**Version:** 1.0 Development Master Context

**Language Stack**

- Core Protocol: C
- Launcher & Simulation Framework: Python
- Transport Layer: UDP
- Serialization: BSON
- Reliability Engine: RL-UACK (Q-Learning)
- Security Layer: PRIoTPS Security Framework
- Architecture Style: Event-Driven Publish-Subscribe Broker

---

# 1. Project Overview

PRIoTPS (Partially Reliable IoT Protocol Secure) is the secure evolution of PRIoTP.

PRIoTP already provides:

- Partial Reliability
- RL-Based UACK
- Publish-Subscribe Communication
- UDP Transport
- Fragmentation/Reassembly
- BSON Serialization
- Congestion Control
- Event-Driven Broker Architecture

PRIoTPS extends PRIoTP with:

- Device Identity
- Secure Handshake
- Session Management
- Payload Encryption
- Payload Authentication
- Secure Fragmentation
- Adaptive Security Policies

The objective is to provide lightweight IoT security while preserving the low-overhead philosophy of PRIoTP.

---

# 2. Core Development Principles

## Rule 1

PRIoTPS is not a separate application.

PRIoTPS is integrated directly into PRTP Core.

Applications must remain unchanged.

Current applications:

```text
PRTP_server

PRTP_client

q_agent_test
```

must continue functioning without API changes.

---

## Rule 2

Security must be transparent.

Applications should never call:

```text
Encrypt()

Decrypt()

Handshake()

KeyExchange()
```

directly.

The protocol core handles everything automatically.

---

## Rule 3

No external cryptographic libraries.

Forbidden:

```text
OpenSSL

WolfSSL

Libsodium

mbedTLS

Botan

Crypto++
```

All cryptographic implementations must exist inside the repository.

---

## Rule 4

IoT-first design.

Always optimize for:

```text
RAM

CPU

Packet Size

Battery Usage

Latency
```

Never optimize for desktop-class hardware.

---

## Rule 5

Security is enabled by default.

There is no insecure mode.

Every session must be protected.

---

# 3. Existing Project Structure

```text
Project Root
│
├── PRTP/
│   ├── src/
│   ├── application/
│   ├── tests/
│   ├── conf/
│   └── sensor.list
│
├── launcher/
├── docs/
├── scripts/
├── load_logs/
│
├── README.md
├── LICENSE
├── project_tree.txt
└── test.py
```

This structure must remain intact.

Security modules should be integrated into PRTP.

---

# 4. PRIoTPS Architecture

```text
                +----------------+
                | Sensors/Data   |
                +--------+-------+
                         |
                         v
                +----------------+
                | Sensor Parsers |
                +--------+-------+
                         |
                         v

+---------+     +----------------------+     +---------+
| Clients |<--->|      PRIoTPS         |<--->| Server  |
+---------+     +----------------------+     +---------+
               | Transport Layer       |
               | BSON Serializer       |
               | Fragmentation         |
               | Congestion Control    |
               | RL-UACK Engine        |
               | Security Framework    |
               +----------+------------+
                          |
                          v
               +----------------------+
               | Security Layer       |
               +----------------------+
               | BLAKE2s              |
               | X25519               |
               | ECIES                |
               | ASCON-AEAD128        |
               | Session Manager      |
               | Secure Handshake     |
               +----------+-----------+
                          |
                          v
               +----------------------+
               | Logging & Statistics |
               +----------------------+
```

---

# 5. Security Architecture

## Identity Generation

Every device must generate:

```text
Random Seed
        ↓
BLAKE2s
        ↓
32-byte Digest
        ↓
X25519
        ↓
Public Key
Private Key
```

Generated once.

Stored locally.

Private keys never leave device.

---

## Handshake

Client:

```text
SUBSCRIBE + Public Key
```

Server:

```text
SUBSCRIBE_ACK
```

Server:

```text
Generate Session Key
```

Server:

```text
Session Key
      ↓
ECIES Layer 1
      ↓
CipherText_1
      ↓
ECIES Layer 2
      ↓
SCT
```

Server sends:

```text
SEH + SCT
```

Client:

```text
Decrypt Layer 1
Decrypt Layer 2
Recover Session Key
```

Client:

```text
Handshake ACK
```

Result:

```text
Secure Session Established
```

---

# 6. Security Extension Header (SEH)

## Handshake SEH

```text
+--------------------------------+
| Flags                          |
+--------------------------------+
| Nonce                          |
+--------------------------------+
| Public Key                     |
+--------------------------------+
| Handshake Payload              |
+--------------------------------+
```

---

## Data SEH

```text
+--------------------------------+
| Flags                          |
+--------------------------------+
| Nonce                          |
+--------------------------------+
| Session ID                     |
+--------------------------------+
| Ciphertext                     |
+--------------------------------+
```

---

# 7. Approved Algorithms

## BLAKE2s

Purpose:

```text
Identity Seed Expansion
Digest Generation
```

Output:

```text
32 Bytes
```

---

## X25519

Purpose:

```text
Identity Generation
Public/Private Keys
```

Output:

```text
32-byte Public Key
32-byte Private Key
```

---

## ECIES

Purpose:

```text
Secure Session Key Delivery
```

Output:

```text
Shareable Cipher Text
```

---

## ASCON-AEAD128

Purpose:

```text
Payload Encryption

Payload Authentication
```

Output:

```text
Ciphertext

Authentication Tag
```

---

# 8. Required Source Layout

Create inside:

```text
PRTP/src/security/
```

Structure:

```text
security/
│
├── blake2s/
│
├── x25519/
│
├── ecies/
│
├── ascon/
│
├── handshake/
│
├── session/
│
├── seh/
│
├── identity/
│
└── security_core/
```

Every module must contain:

```text
header file

source file

unit tests

documentation
```

---

# 9. Secure Data Flow

Current PRTP:

```text
Payload
↓
BSON
↓
Transport
↓
UDP
```

PRIoTPS:

```text
Payload
↓
BSON
↓
Fragmentation
↓
ASCON Encrypt
↓
SEH
↓
Transport
↓
UDP
```

Receiving:

```text
UDP
↓
Transport
↓
SEH Validation
↓
ASCON Verify
↓
ASCON Decrypt
↓
Reassembly
↓
BSON Decode
↓
Application
```

---

# 10. Session Management

Each active client owns:

```c
typedef struct
{
    uint32_t session_id;

    uint8_t session_key[16];

    uint64_t created_at;

    uint64_t last_seen;

    uint8_t active;

} priotps_session_t;
```

Responsibilities:

```text
Session Creation

Session Lookup

Session Expiry

Session Renewal

Session Deletion
```

---

# 11. RL-UACK Integration

Current RL Actions:

```text
Reliable

Unreliable

Drop
```

Future PRIoTPS Actions:

```text
Reliable

Unreliable

Drop

Rekey

Refresh Session

Increase Security

Reduce Security
```

Security must be designed so future RL expansion is possible.

Do not hardcode limitations.

---

# 12. Launcher Responsibilities (Python)

Directory:

```text
launcher/
```

Launcher is not part of protocol core.

Launcher responsibilities:

```text
Sensor Simulation

Node Deployment

Experiment Execution

Traffic Generation

Benchmark Automation

Result Collection

Log Cleanup

Performance Monitoring
```

Launcher must never implement protocol logic.

Protocol logic belongs exclusively inside C codebase.

---

# 13. Coding Standards

## C Standards

Target:

```text
C99
```

Requirements:

```text
Modular

Portable

Cross-platform

Minimal heap allocations

No hidden globals

No cryptographic shortcuts
```

---

## Python Standards

Target:

```text
Python 3.11+
```

Requirements:

```text
Type Hints

Structured Logging

Modular Design

No Protocol Logic
```

---

# 14. Testing Requirements

Every module requires:

## Unit Tests

```text
BLAKE2s

X25519

ECIES

ASCON

SEH

Session Manager
```

## Integration Tests

```text
Client ↔ Server Handshake

Encrypted Publish

Encrypted Subscribe

Fragmentation

Reassembly
```

## Security Tests

```text
Replay Attack

Tampering

Wrong Key

Expired Session

Malformed Packet
```

---

# 15. Performance Targets

PRIoTPS must preserve PRIoTP philosophy.

Primary goals:

```text
Low Latency

Low Overhead

Low Memory Usage

Low CPU Usage

Scalable Broker Design

Resource-Constrained Device Support
```

Security overhead should remain minimal and measurable.

---

# 16. Definition of Done

PRIoTPS is considered complete when:

- Secure identities are generated automatically.
- Secure handshakes operate successfully.
- Session keys are exchanged securely.
- Payloads are encrypted using ASCON-AEAD128.
- Payloads are authenticated.
- Fragmentation works with encrypted payloads.
- UACK/NACK continues functioning.
- Existing applications run without modification.
- No external crypto libraries are required.
- Launcher tools continue operating normally.
- Security is enabled by default.
- All tests pass.

Final Result:

```text
PRTP
  ↓
PRIoTPS

Partial Reliability
+
Adaptive Intelligence
+
Lightweight Security

= Secure IoT Communication Framework
```
