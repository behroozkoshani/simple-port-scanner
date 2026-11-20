# Safe and Authorized Use

This repository provides a learning-oriented TCP connect scanner. Only scan hosts and ports when you own them or have explicit written permission, and honor the approved time, IP ranges, and traffic limits of any assessment.

- By default the executable only accepts IPv4 loopback addresses (`127.0.0.0/8`).
- `--allow-remote` is an opt-in acknowledgment of authorization. It is not a proof of ownership, a permission system, or a guarantee that a scan is lawful.
- Run the `scripts/demo_local.py` examples to see real program output without contacting an external host.
- Keep concurrency and timeout values conservative when testing a shared or resource-constrained environment.
- Do not upload sensitive target inventories, real internal IP information, network test logs, secrets, or credentials to public repositories.
- Port states and service hints are limited observations, not security verdicts or application identification.

Only the local demo and unit tests are represented by the included screenshot evidence.
