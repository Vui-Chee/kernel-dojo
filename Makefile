# 1) integration tree
#       https://git.kernel.org/pub/scm/linux/kernel/git/next/linux-next.git
# 2) subsystem of interest in
#       https://git.kernel.org/pub/scm/linux/kernel/git/netdev/net.git linux-netdev
KERNEL_VERSION ?= 6.18.6
KERNEL_REMOTE ?= git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
KERNEL_DIR ?= linux-$(KERNEL_VERSION)

# sudo apt install clang lld llvm
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
	./build.sh -d $(KERNEL_DIR) -S

.PHONY: clean
clean:
	@echo "Cleaning the kernel build..."	
	cd $(KERNEL_DIR) && make clean

.PHONY: scope
scope:
	@echo "Generating cscope database..."	
	make -C $(KERNEL_DIR) cscope
	@echo "Starting cscope..."	
	cd $(KERNEL_DIR)  && cscope -d
