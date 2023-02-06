#!/bin/bash
# Script outline to install and build kernel.
# Author: Siddhant Jajoo.

set -e
set -u

OUTDIR=/tmp/aeld
KERNEL_REPO=git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
KERNEL_VERSION=v5.1.10
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
	echo "starting the build procedure"
	echo "Cleaning the source tree"
        make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} mrproper		#cleans the source tree
        echo "Build the default configuration"
        make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} defconfig		#generates a default configuration file for build
        echo "Build the Kernel Image in multiple core"
        make -j7 ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} all            	#use multiple CPU to build
        echo "Build the module"
        make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} modules		#builds module
        echo "Build the Device Tree"
        make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} dtbs 		#builds device tree
        echo "Completing the build procedure"
fi

echo "Adding the Image in outdir"	
cp /tmp/aeld/linux-stable/arch/arm64/boot/Image ${OUTDIR}


echo "Creating the staging directory for the root filesystem"
cd "$OUTDIR"
if [ -d "${OUTDIR}/rootfs" ]
then
	echo "Deleting rootfs directory at ${OUTDIR}/rootfs and starting over"
    sudo rm  -rf ${OUTDIR}/rootfs
fi

# TODO: Create necessary base directories
echo "Creating rootfs directory and necessary dependencies"
mkdir -p "${OUTDIR}/rootfs"
cd "${OUTDIR}/rootfs"
mkdir bin dev etc home lib lib64 proc sbin sys tmp usr var
mkdir usr/bin usr/lib usr/sbin
mkdir -p var/log
echo "Completion of base directory"


cd "$OUTDIR"
if [ ! -d "${OUTDIR}/busybox" ]
then
git clone git://busybox.net/busybox.git
    cd busybox
    git checkout ${BUSYBOX_VERSION}
    
    # TODO:  Configure busybox
    make distclean
    make defconfig
else
    cd busybox
fi

# TODO: Make and install busybox
echo "make and install busy Box"
make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE}
make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} CONFIG_PREFIX="${OUTDIR}/rootfs" install 
echo "Installing busy box Complete"



echo "Library dependencies"
cd ${OUTDIR}/rootfs
${CROSS_COMPILE}readelf -a bin/busybox | grep "program interpreter"
${CROSS_COMPILE}readelf -a bin/busybox | grep "Shared library"

echo "adding library dependcies"
# TODO: Add library dependencies to rootfs
export SYSROOT=$(${CROSS_COMPILE}gcc -print-sysroot)

cp -a $SYSROOT/lib/ld-linux-aarch64.so.1 lib		#ok
cp -a $SYSROOT/lib64/ld-2.31.so lib64			#ok
cp -a $SYSROOT/lib64/libc.so.6 lib64			#ok
cp -a $SYSROOT/lib64/libc-2.31.so lib64			#ok
cp -a $SYSROOT/lib64/libm.so.6 lib64			#ok
cp -a $SYSROOT/lib64/libm-2.31.so lib64			#ok
cp -a $SYSROOT/lib64/libresolv-2.31.so lib64
cp -a $SYSROOT/lib64/libresolv.so.2 lib64



# TODO: Make device nodes
echo "Make device nodes"
sudo mknod -m 666 dev/null c 1 3
sudo mknod -m 600 dev/console c 5 1

# TODO: Clean and build the writer utility
echo "clean and build writer utility"
cd $FINDER_APP_DIR
make clean
make CROSS_COMPILE=$CROSS_COMPILE

# TODO: Copy the finder related scripts and executables to the /home directory
# on the target rootfs
echo "copying realted finder scripts to /tmp/aesd/rootfs/home"
cp writer ${OUTDIR}/rootfs/home
cp writer.o ${OUTDIR}/rootfs/home
cp writer.c ${OUTDIR}/rootfs/home
cp start-qemu-terminal.sh ${OUTDIR}/rootfs/home
cp start-qemu-app.sh ${OUTDIR}/rootfs/home
cp manual-linux.sh ${OUTDIR}/rootfs/home
cp Makefile ${OUTDIR}/rootfs/home
cp finder-test.sh ${OUTDIR}/rootfs/home
cp finder.sh ${OUTDIR}/rootfs/home
cp autorun-qemu.sh ${OUTDIR}/rootfs/home
cp dependencies.sh ${OUTDIR}/rootfs/home
mkdir ${OUTDIR}/rootfs/home/conf
cp conf/assignment.txt ${OUTDIR}/rootfs/home/conf
cp conf/username.txt ${OUTDIR}/rootfs/home/conf


# TODO: Chown the root directory
echo "Chown the root directory"
cd ${OUTDIR}/rootfs
sudo chown -R root:root *


# TODO: Create initramfs.cpio.gz
echo "Create initramfs.cpio.gz"
find . | cpio -H newc -ov --owner root:root > ../initramfs.cpio
cd ..
gzip initramfs.cpio
#mkimage -A arm -O linux -T ramdisk -d initramfs.cpio.gz uRamdisk



