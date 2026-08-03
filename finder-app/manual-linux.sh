#!/bin/bash
# Script outline to install and build kernel.
# Author: Siddhant Jajoo.

set -e
set -u

OUTDIR=/tmp/aeld
KERNEL_REPO=git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
KERNEL_VERSION=v5.15.163
BUSYBOX_VERSION=1_33_1
FINDER_APP_DIR=$(realpath $(dirname $0))
ARCH=arm64
CROSS_COMPILE=aarch64-none-linux-gnu-

if [ $# -lt 1 ]
then
	echo "Using default directory ${OUTDIR} for output"
else
	OUTDIR=$1
	echo "Using passed directory ${OUTDIR} for output"
fi

mkdir -p ${OUTDIR}
if [ ! $? -eq 0 ]
then
    echo "ERROR creating output directory"
    exit 1
fi

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/linux-stable" ]; then
    #Clone only if the repository does not exist.
	echo "CLONING GIT LINUX STABLE VERSION ${KERNEL_VERSION} IN ${OUTDIR}"
	git clone ${KERNEL_REPO} --depth 1 --single-branch --branch ${KERNEL_VERSION}
fi
if [ ! -e ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ]; then
    cd linux-stable
    echo "Checking out version ${KERNEL_VERSION}"
    git checkout ${KERNEL_VERSION}

    # TODO: Add your kernel build steps here
    #clean
    echo "================== CLEANING =================="
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} mrproper
    # build defconfig
    echo "================== BUILDING DEFCONFIG =================="
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} defconfig
    # build kernel image
    echo "================== BUILDING KERNEL IMAGE =================="
    make -j4 ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} all
    # build modules
    #echo "================== BUILDING MODULES =================="
    #make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} modules
    # build device tree
    echo "================== BUILDING DEVICE TREE =================="
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} dtbs
fi

echo "Adding the Image in outdir"
cp ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ${OUTDIR}/

echo "Creating the staging directory for the root filesystem"
cd "$OUTDIR"
if [ -d "${OUTDIR}/rootfs" ]
then
	echo "Deleting rootfs directory at ${OUTDIR}/rootfs and starting over"
    sudo rm  -rf ${OUTDIR}/rootfs
fi

# TODO: Create necessary base directories
mkdir -p "${OUTDIR}/rootfs"
cd rootfs
mkdir -p bin dev etc home lib lib64 proc sbin sys tmp usr var
mkdir -p usr/bin usr/lib usr/sbin home/conf
mkdir -p var/log

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/busybox" ]
then
git clone git://busybox.net/busybox.git
    cd busybox
    git checkout ${BUSYBOX_VERSION}
    # TODO:  Configure busybox
    echo "================== CONFIGURING BUSY BOX =================="
    make distclean
    make defconfig
else
    cd busybox
fi

# TODO: Make and install busybox
echo "================== MAKING AND INSTALLING BUSY BOX =================="
make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE}
make CONFIG_PREFIX=${OUTDIR}/rootfs ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} install
cd ${OUTDIR}/rootfs
sudo rm -rf lib
sudo ln -sf lib64 lib


echo "================== LIBRARY DEPENDENCIES =================="
${CROSS_COMPILE}readelf -a bin/busybox | grep "program interpreter"
${CROSS_COMPILE}readelf -a bin/busybox | grep "Shared library"

# TODO: Add library dependencies to rootfs
cd "${OUTDIR}/rootfs"
SYSROOT=/opt/toolchains/arm-gnu-toolchain-13.3.rel1-x86_64-aarch64-none-linux-gnu/aarch64-none-linux-gnu
cp ${SYSROOT}/libc/lib/ld-linux-aarch64.so.1 lib64/ld-linux-aarch64.so.1
cp ${SYSROOT}/libc/lib64/libm.so.6 lib64/libm.so.6
cp ${SYSROOT}/libc/lib64/libresolv.so.2 lib64/libresolv.so.2
cp ${SYSROOT}/libc/lib64/libc.so.6 lib64/libc.so.6

# TODO: Make device nodes
sudo mknod -m 666 dev/console c 5 1
sudo mknod -m 666 dev/null c 1 3

# TODO: Clean and build the writer utility
cd "${FINDER_APP_DIR}"
echo "Cleanning writer utility"
make clean
echo "Building writer uttility"
make CROSS_COMPILE=${CROSS_COMPILE}

# TODO: Copy the finder related scripts and executables to the /home directory
# on the target rootfs
if [ $? -eq 0 ]
then
    chmod +x finder.sh
    cp autorun-qemu.sh dependencies.sh finder-test.sh finder.sh start-qemu-app.sh start-qemu-terminal.sh writer "${OUTDIR}/rootfs/home"
    cp conf/assignment.txt conf/username.txt "${OUTDIR}/rootfs/home/conf/"
else
    echo "ERROR: Writer utility cannot be buit!"
fi
# TODO: Chown the root directory
cd "${OUTDIR}"
sudo chown -R root:root rootfs
# TODO: Create initramfs.cpio.gz
echo "================== CREATING RAM DISK =================="
cd rootfs
find . | cpio -H newc -ov --owner root:root > ${OUTDIR}/initramfs.cpio
cd "${OUTDIR}"
gzip -f initramfs.cpio