# Security Policy: leptonai/dummy_dmesg

## Reporting a Vulnerability

Do not open a public GitHub issue or pull request for a suspected vulnerability, and do not post kernel logs from real hosts in a public place.

Report vulnerabilities privately through one of these channels:

1. **NVIDIA Vulnerability Disclosure Program (preferred):** [NVIDIA Product Security](https://www.nvidia.com/en-us/security/), using the [vulnerability report form](https://www.nvidia.com/en-us/security/report-vulnerability/).
2. **Email:** [psirt@nvidia.com](mailto:psirt@nvidia.com). Use the [NVIDIA public PGP key](https://www.nvidia.com/en-us/security/pgp-key) when the report contains sensitive evidence.
3. **GitHub private vulnerability reporting:** This repository's **Security** tab, then **Report a vulnerability**.

Include the repository name, the affected commit, the Linux kernel version, the permissions on the device node, the vulnerability type, reproduction steps, proof of concept if available, and the impact. Remove hostnames, credentials, and customer data from log excerpts. Use the private channel to share anything that is itself sensitive.

NVIDIA PSIRT acknowledges receipt, validates the report, and coordinates remediation and disclosure. For this repository, remediation is a source change on `main`. There is no packaged release to patch.

## Security Architecture and Context

`dummy_dmesg` is a Linux kernel module that writes caller-supplied text to the kernel log. It exists so that software that reads `dmesg` can be tested with synthetic messages, such as the GPU Xid example in `README.md`. It is a loadable kernel module (driver). It has no network listener, no user-space daemon, and no release.

The maintained copy is the `main` branch. The files are:

- `dummy_dmesg.c`, the module. It registers a character device named `dummy-dmesg` with a dynamically assigned major number and implements one operation, `dev_write`.
- `Makefile`, which builds the module against the running kernel's headers. Its `install` and `uninstall` targets run `sudo insmod` and `sudo rmmod`.
- `README.md`, which shows how to build the module, load it, create `/dev/dummy_dmesg` with `mknod`, and write a message.

Its security responsibility is to copy user-supplied bytes into kernel memory safely and to print them as data. `dev_write` limits each write to 999 bytes, copies it with `copy_from_user` into a static 1,000-byte buffer, appends a NUL terminator, and emits it with `printk(KERN_INFO "%s\n", msg)`. The fixed format string means the message is never interpreted as a format. A mutex taken with `mutex_trylock` guards the buffer, and a contended write returns `-EBUSY`.

The boundaries are:

- The write interface between user space and the kernel, in `dev_write`.
- The device node. The module does not create it and does not set its owner or mode. Whoever runs `mknod` decides who can write.
- Module loading, which requires root on the host.
- The kernel log, which the module writes into and which monitoring and log-forwarding software reads.

**Repository Exposure Classification:** Public. Basis: `github.com/leptonai/dummy_dmesg` is a public GitHub repository; this document is written to public-safe detail.

**Service Exposure Classification:** External / Regulated (medium confidence). Basis: the source is publicly readable and openly licensed; it is a test utility with no release, package, or network service, and it runs with kernel privileges on any host where an operator loads it. The source does not establish where the module is deployed.

### Threat Model

1. **Forged kernel log messages:** Any process allowed to write to the device node can put arbitrary text into the kernel log, including newlines and text that imitates a real GPU Xid or kernel failure. Software that parses `dmesg` cannot tell from the text that the message is synthetic, so it may raise alerts or start automated repair.
2. **Kernel memory safety in `dev_write`:** The handler runs in kernel context on a user-supplied pointer and length. Safety depends on the length clamp to `BUF_LEN - 1`, the `copy_from_user` error check, and `msg_mutex` around the shared static buffer. A defect in any of them is a kernel memory-corruption or disclosure bug, not a log-content issue.
3. **Kernel log flooding:** `dev_write` has no rate limit. Repeated writes fill the kernel ring buffer, push out real messages, and load log collectors. The per-write size cap and the mutex do not bound the message rate.
4. **A device node open to unprivileged users or containers:** The module leaves permissions to the operator. A permissive mode on `/dev/dummy_dmesg`, or a node passed into a container, extends threats 1 and 3 to every process that can open it.
5. **An untrusted module binary:** `make install` loads `dummy_dmesg.ko` with `sudo insmod`. Modified source, or a `.ko` file built somewhere else, runs with full kernel privileges. This repository publishes no signed or prebuilt module.
6. **Sensitive data in the kernel log:** Bytes written to the device become kernel log content and can be forwarded off the host. A credential or customer value written as a test message is disclosed to every log consumer.

Message injection is the module's purpose. That does not make memory corruption, privilege escalation, or unauthorized access to the device acceptable.

### Critical Security Assumptions

- Only a trusted administrator loads the module and creates the device node. Write access is limited to the users who are meant to inject test messages. The module adds no authorization of its own.
- The module is loaded on test hosts, or on hosts where the monitoring and repair systems expect synthetic messages. It is not left loaded in production.
- The host kernel enforces device permissions and the module-loading policy. This repository does not sign the module.
- The build host, its kernel headers, and the resulting `.ko` file are trusted. The module is built for the kernel it is loaded into.
- Consumers of the kernel log treat message text as untrusted input.
- Host log permissions, retention, and forwarding rules protect whatever is written.

## Supported Versions

This repository has no tags and no releases. `main` is the only maintained copy. Security reports should name the commit and the kernel version, not a version number.
