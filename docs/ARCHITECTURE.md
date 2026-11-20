# Architecture and Network Behavior

## Goal

Create a lightweight, auditable C++20 TCP connect scanner that uses standard OS sockets, bounded parallelism, and a reproducible localhost lab. This is a defensive learning implementation; it does not identify software vulnerabilities.

## Data flow

```text
Command line (--target, --ports, --threads, --timeout-ms)
       |
       v
Validate CLI input --> normalize / sort / deduplicate TCP ports
       |
       v
Resolve target IPv4 address --> deny non-loopback unless --allow-remote
       |
       v
Bounded worker pool (up to 256 worker threads)
       |
       +--> nonblocking TCP socket --> connect() --> select(timeout)
       |                                   |
       |                                   +--> getsockopt(SO_ERROR)
       |                                            |
       +--------------------------------------------+
       |
       v
Per-port state classification (OPEN / CLOSED / FILTERED / ERROR)
       |
       v
Ordered results --> table / JSON / CSV --> optional output file
```

## Key implementation decisions

**TCP connect instead of raw SYN:** A successful normal TCP handshake does not require raw packet privileges. The approach is easier to reproduce in a local lab and is straightforward to verify against an actual listening socket.

**Nonblocking sockets and `select()`:** `connect()` can take time when traffic is dropped or lost. Each worker uses `select()` for a per-port timeout and reads `SO_ERROR` to distinguish successful connections from connection refusal and unexpected OS-level errors.

**Bounded concurrency:** Worker threads pull indices from an atomic counter and write to their assigned result slots. The limit is configurable from 1 to 256, avoiding unbounded thread creation for large port specifications. The final table is sorted because it is written in the already sorted port sequence, regardless of connection completion order.

**IPv4 address resolution:** `getaddrinfo()` resolves a hostname to IPv4; the first returned IPv4 address is selected. The address is tested for the 127.0.0.0/8 loopback range before any connection is attempted. Hosts with multiple addresses are not exhaustively evaluated.

**State semantics:**

| Result | Signal | Interpretation |
| --- | --- | --- |
| OPEN | Connect succeeds or `SO_ERROR == 0` | TCP listener or intermediary accepted the connection |
| CLOSED | Connection refused | No listener accepted this connection at the time |
| FILTERED | Connection attempt times out | Target/path may be filtering, dropping, or unresponsive; this is a hypothesis |
| ERROR | Other OS failure | Invalid/unreachable network condition or socket failure; see detail |

The port-to-service names are a small conventional mapping, not banners or verified identities. No application messages are transmitted by the scanner.

## Trust and safety boundaries

- The demo script uses **127.0.0.1 only** and binds ephemeral local ports.
- Remote scans require the user to confirm authorization with an explicit flag.
- That flag is a voluntary safety check, **not an authorization enforcement service**.
- The code contains no stealth, exploitation, password guessing, or vulnerability fingerprinting modules.
- Output reports may still contain target and port information; treat real scan reports as sensitive and do not commit them without permission.

## Non-goals and possible future work

IPv6 support; stronger host resolution controls; machine-readable error codes; cancellable scans; rate limiting; explicit connection budgets; safer detailed logging; Python package-free browserless report viewing; and benchmarking the performance with synthetic local services. Future features should remain scoped to explicitly authorized environments.
