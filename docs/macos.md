# Running in MacOS

## Download UTM

[utm](https://getutm.app)

## Download VM image

[ubuntu images](https://cdimage.ubuntu.com/ubuntu/releases/?_gl=1*dp2knn*_gcl_au*MjYzOTgzODc0LjE3OTQ3OTc2MDMuODk1MzM1MzUyLjE3OTQ3OTc2MDMuMTc5NDc5NzYwNC40NjY0MDUyMzEuMTc5NDc5NzYwMy4xNzk0Nzk3NjA0)

## Create VM image

Use virtualization on real hardware (arm64 or aarch64). Assign disk space accordingly.

Be sure to delete the CD/DVD drive in order to start the server without returning back to install menu.

### Resources

From my experiments, it is best to assign all cores available to the VM. I am using `m5 pro` with 18 cores. Use
`4GB` to `8GB` is  sufficient for a build.

It took around `5 min 55 secs` using 18 cores and 6GB.

## Resize disk after image creation

```sh
# 1. Resize the LVM physical volume to recognize the larger partition size
sudo pvresize /dev/vda3

# 2. Extend the LVM logical volume to use 100% of the remaining free space
sudo lvextend -l +100%FREE /dev/mapper/ubuntu--vg-ubuntu--lv

# 3. Resize the filesystem so the OS can use the expanded space
sudo resize2fs /dev/mapper/ubuntu--vg-ubuntu--lv
```

## Install dependencies

```sh
sudo apt update
sudo apt install -y wget

wget https://apt.llvm.org/llvm.sh
chmod +x llvm.sh
sudo ./llvm.sh <version> # eg. 23
```

```sh
# Obviously you should specify the right clang and llvm versions based on the kernel version.
sudo apt install -y \
    clang-23 \
    lld-23 \
    llvm-23 \
    llvm-23-tools \
    libclang-rt-23-dev \
    dwarves \
    flex \
    bison \
    bc \
    libelf-dev \
    libssl-dev \
    libncurses-dev \
    build-essential
```

## Build

The args will be passed to `tools/build.sh`

```sh
make build LLVM="-<version>" CC="clang-<version>"
```
