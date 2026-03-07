# Kernel Dojo

A hands-on environment for learning Linux kernel development. Build, run, and hack on the kernel inside QEMU — no risk to your host system.

## What's included

- **Automated kernel build** — fetches kernel source and builds a QEMU-bootable image with one command
- **QEMU runner** — boots the kernel with networking, SSH, and module loading pre-configured
- **Example modules** — annotated kernel modules to learn from and extend
- **Scaffolding** — quickly create new modules based on a working template

## Prerequisites

Install dependencies for your distro:

```sh
make install
```

Supports apt, dnf, pacman, and zypper. You'll also need `clang`, `lld`, and `llvm` installed separately.

## Getting started

### 1. Get the kernel source

```sh
make clone                  # clones linux-stable at the pinned version
```

The default is **Linux 6.18.6**. Override with:

```sh
make clone KERNEL_VERSION=6.12.0
```

### 2. Build the kernel

```sh
./tools/build.sh -d linux-6.18.6
```

This produces a bootable image at `linux-6.18.6/arch/x86/boot/bzImage`.

### 3. Boot in QEMU

```sh
make run
```

Connect via SSH on port 52222. Override CPU count and memory:

```sh
NRCPU=4 MEMORY=8192 make run
```

## Writing kernel modules

### Create a new module

```sh
make imod name=my_module
```

This copies the `bare-module` template into `examples/my_module/`.

### Build a module

```sh
cd examples/my_module
make -C ../../linux-6.18.6 M=$(pwd) modules
```

### Example modules

| Module | Description |
|---|---|
| `bare-module` | Minimal loadable module — a clean starting point |
| `process-times` | Captures per-process CPU times via a kernel module |

## Project layout

```
kernel-dojo/
├── examples/           # Kernel modules
│   ├── bare-module/    # Template module
│   └── process-times/  # Example: process CPU time tracking
├── tools/
│   ├── build.sh        # Kernel build script (clang/LLVM)
│   ├── start_qemu.sh   # QEMU launcher with networking
│   ├── genmake.sh      # Makefile generator for modules
│   └── host_lib.sh     # Shared shell utilities
├── readings/           # Notes and references
├── linux-6.18.6/       # Kernel source (after make clone)
└── Makefile
```

## Useful make targets

| Target | Description |
|---|---|
| `make install` | Install build dependencies |
| `make clone` | Clone the kernel source |
| `make run` | Boot the kernel in QEMU |
| `make scope` | Build and open a cscope index of the kernel |
| `make imod name=<n>` | Scaffold a new module named `<n>` |
| `make clean` | Clean the kernel build |
