# Security

## Reporting Policy

Do not open public issues for security vulnerabilities or publish sensitive
kernel logs. Report suspected vulnerabilities privately through the
[NVIDIA Vulnerability Disclosure Program](https://www.nvidia.com/en-us/security/report-vulnerability/)
or email [psirt@nvidia.com](mailto:psirt@nvidia.com). Use the
[NVIDIA PSIRT PGP key](https://www.nvidia.com/en-us/security/pgp-key) for sensitive
email reports.

Include the affected revision, Linux kernel version, device-node permissions,
reproduction steps, and expected impact. NVIDIA PSIRT coordinates triage and
disclosure under its
[response policies](https://www.nvidia.com/en-us/security/psirt-policies/).

## Architecture Context

`dummy_dmesg` is a small Linux kernel module for writing synthetic messages to
the kernel log. `dummy_dmesg.c` registers a character device named `dummy-dmesg`;
the README shows creating a device node at `/dev/dummy_dmesg` and writing example
GPU Xid messages to it. The Makefile builds the module against the host kernel
and provides commands to load and unload it.

The security boundary is the character-device write interface between user space
and kernel space. `dev_write` copies user-supplied bytes into a shared 1,000-byte
buffer, limits each write to 999 bytes, appends a NUL terminator, and emits the
message with `printk(KERN_INFO "%s\n", msg)`. A mutex serializes buffer access;
contended writes return `-EBUSY`. The fixed format string treats the supplied
message as data.

The module has no network listener or application-level authentication. Access
depends on Linux device-node permissions and host security controls. Its output
enters the host kernel log and may be collected by monitoring or log-forwarding
systems. The source does not establish where the module is currently deployed.

## Threat Model

- **Forged diagnostic messages:** a process permitted to write to the device can
  inject arbitrary text, including newlines and messages resembling real GPU or
  kernel failures. Downstream consumers must not treat these messages as proof
  of a hardware fault or authorization for automated repair.
- **Log flooding and availability:** repeated writes to `dev_write` can generate
  kernel log traffic and consume logging or monitoring resources. The per-write
  size limit and mutex do not impose a message rate limit.
- **Kernel memory-safety failures:** the user-pointer copy and buffer handling
  execute in kernel context. Bounds checks, `copy_from_user` error handling, and
  synchronization are security-relevant; defects that cross this boundary remain
  in scope even though message injection is intentional.
- **Sensitive data disclosure:** bytes written to the device become kernel log
  content and may reach external collectors. Do not send credentials or other
  sensitive data through the device.
- **Module or build compromise:** modified source or an untrusted `.ko` file runs
  with kernel privileges when loaded. Review the source and protect the build
  environment and module artifact before installation.

The intended ability to inject messages does not accept privilege escalation,
memory corruption, unauthorized device access, or uncontrolled downstream
effects as safe. This policy grants no blanket scanning exemptions.

## Critical Security Assumptions

- A trusted administrator controls module loading, unloading, and device-node
  creation. Restrict write permissions to users authorized to inject kernel logs;
  do not expose the device to untrusted users or containers.
- The host kernel enforces device access and module-loading policy. This module
  adds no independent authorization or isolation layer.
- Operators use a trusted kernel build environment and a module compatible with
  the target kernel. Loading the module places it inside the kernel trust boundary.
- Monitoring and repair systems account for synthetic messages on hosts where
  this module is enabled. Test message injection in a controlled environment
  before connecting it to operational automation.
- Host log permissions, retention, and forwarding policies protect emitted data.
