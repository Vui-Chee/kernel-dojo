# 1) integration tree
#       https://git.kernel.org/pub/scm/linux/kernel/git/next/linux-next.git
# 2) subsystem of interest in
#       https://git.kernel.org/pub/scm/linux/kernel/git/netdev/net.git linux-netdev
KERNEL_VERSION ?= 6.18.6
KERNEL_REMOTE ?= git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
KERNEL_DIR ?= $(realpath linux-$(KERNEL_VERSION))
CC := clang
CFLAGS := -Wall -O2 -I$(KERNEL_DIR)/tools/testing/selftests

# LLVM follows how kernel is built using build.sh
MAKEFLAGS += KCFLAGS="-Wno-error=missing-prototypes" LLVM=1 C=2 -j$(nproc)
BUILD ?= ./tools/build.sh
RUN ?= ./tools/start_qemu.sh
KERNEL_IMAGE ?= $(KERNEL_DIR)/arch/x86/boot/bzImage
DEBUG ?= true

M_DIR ?= modules/*
M_DIR := $(realpath $(M_DIR))

# Allow commands to operate only these files.
EXPANDED_SRCS  := $(wildcard $(SRCS))
FILTERED_SRCS := $(filter-out %.mod.c,$(EXPANDED_SRCS))

export KROOT

ifeq ($(shell which apt 2>/dev/null), /usr/bin/apt)
	INSTALL_CMD = sudo apt update && sudo apt install -y
	PACKAGES = build-essential libncurses-dev bison flex libssl-dev libelf-dev qemu-system-x86 cpio libdw-dev
else ifeq ($(shell which dnf 2>/dev/null), /usr/bin/dnf)
	INSTALL_CMD = sudo dnf groupinstall -y "Development Tools" && sudo dnf install -y
	PACKAGES = ncurses-devel bison flex openssl-devel elfutils-libelf-devel qemu-system-x86 cpio elfutils-devel
else ifeq ($(shell which pacman 2>/dev/null), /usr/bin/pacman)
	INSTALL_CMD = sudo pacman -Sy --needed --noconfirm
	PACKAGES = base-devel ncurses bison flex openssl libelf qemu-base cpio elfutils
else ifeq ($(shell which zypper 2>/dev/null), /usr/bin/zypper)
	INSTALL_CMD = sudo zypper install -y
	PACKAGES = -t pattern devel_kernel devel_basis ncurses-devel bison flex libopenssl-devel libelf-devel qemu-x86 cpio libdw-devel
else
	$(error "No supported package manager found (apt, dnf, pacman, zypper). Please install dependencies manually.")
endif

echo:
	@echo $(KERNEL_DIR)

.PHONY: install
install:
	@echo "Detected package manager. Installing dependencies..."
	$(INSTALL_CMD) $(PACKAGES)

.PHONY: clone
clone:
	@if [ -d $(KERNEL_DIR) ]; then \
		echo "$(KERNEL_DIR) already exists."; \
	else \
		echo "cloning..."; \
		git clone --depth 1 --branch v$(KERNEL_VERSION) $(KERNEL_REMOTE) $(KERNEL_DIR); \
	fi

.PHONY: build
build:
	@echo "Building the kernel..."
	$(BUILD) -d $(KERNEL_DIR) -S

run:
	@echo "Running the kernel in QEMU..."
	START_DIR=$(realpath $(M_DIR)) $(RUN) -m $(KERNEL_IMAGE)

.PHONY: clean
clean:
	@echo "Cleaning the kernel build..."
	cd $(KERNEL_DIR) && $(MAKE) clean

.PHONY: scope
scope:
	@echo "Generating cscope database..."
	$(MAKE) -C $(KERNEL_DIR) cscope
	@echo "Starting cscope..."
	cd $(KERNEL_DIR) && cscope -d

.PHONY: mod
mod:
	@for dir in $(M_DIR); do \
		[ -d $$dir ] || continue; \
		echo "Building $$dir..."; \
		cd $$dir && \
			DEBUG=$(DEBUG) KROOT=$(KERNEL_DIR) ../../tools/genmake.sh && \
			if [ -f Makefile ]; then $(MAKE); fi; \
		cd $(CURDIR); \
	done

.PHONY: cmod
cmod:
	@for dirs in $(M_DIR); 	do \
	if [ -f $$dirs/Makefile ] ; then \
		$(MAKE) -C $$dirs clean ; fi ;\
	done

.PHONY: fmt
fmt:
	clang-format -style=file:$(KERNEL_DIR)/.clang-format -i --verbose $(FILTERED_SRCS)

.PHONY: lint
lint:
	$(KERNEL_DIR)/scripts/checkpatch.pl --file --no-tree $(FILTERED_SRCS)

.PHONY: test
test:
	@for f in $(M_DIR)tests/*.c;do \
		b="$${f%.c}"; \
		rm -rf "$$b" "$$b.d"; \
		$(CC) $(CFLAGS) "$$b.c" -o "$$b"; \
		./"$$b"; \
		rm -rf "$$b" "$$b.d"; \
	done
