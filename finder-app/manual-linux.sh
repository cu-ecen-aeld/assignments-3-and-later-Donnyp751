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
CROSS_COMPILE_BASE=aarch64-none-linux-gnu
CROSS_COMPILE=${CROSS_COMPILE_BASE}-
ROOTFS_DIRECTORIES=("/bin" "/lib" "/lib64" "/usr" "/var" "/etc" "/home" "/home/conf" "/root" "/dev" "/tmp" "/proc" "/sys" "/usr/bin" "usr/lib" "/usr/sbin" "/var/log")
ROOTFS=${OUTDIR}/rootfs
TOOLCHAIN_ROOTFS=$(${CROSS_COMPILE}gcc --print-sysroot)

if [ $# -lt 1 ]
then
	echo "Using default directory ${OUTDIR} for output"
else
	OUTDIR=$1
	echo "Using passed directory ${OUTDIR} for output"
fi

ROOTFS=${OUTDIR}/rootfs

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
    cp ${FINDER_APP_DIR}/../conf/.k_config ${OUTDIR}/linux-stable/.config
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE}
fi

echo "Adding the Image in outdir"
cp ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ${OUTDIR}

echo "Creating the staging directory for the root filesystem"
cd "$OUTDIR"
if [ -d "${OUTDIR}/rootfs" ]
then
	echo "Deleting rootfs directory at ${OUTDIR}/rootfs and starting over"
    sudo rm  -rf ${OUTDIR}/rootfs
fi


for dir in "${ROOTFS_DIRECTORIES[@]}"; do
    target_dir=${ROOTFS}/$dir
    if [ ! -d $target_dir ]; then
        mkdir -p $target_dir
    else
        echo "$target_dir already exists"
    fi
done

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/busybox" ]
then
git clone git://busybox.net/busybox.git
    cd busybox
    git checkout ${BUSYBOX_VERSION}
    cp ${FINDER_APP_DIR}/../conf/.bb_config ${OUTDIR}/busybox/.config
else
    cd busybox
fi

echo "Building Busybox"

make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} install CONFIG_PREFIX=${ROOTFS}

#echo "Library dependencies"
interpreter=$(${CROSS_COMPILE}readelf -l ${ROOTFS}/bin/busybox | awk '/interpreter/ {print $NF}' | tr -d '[]')
interpreter=$(${CROSS_COMPILE}readelf -a ${ROOTFS}/bin/busybox | grep "program interpreter" |  grep -oP "/?[a-zA-Z0-9_-]+(/[a-zA-Z0-9_-]+)*\.so\.[0-9]+")
echo "Interpreter: $interpreter"
libraries=$(${CROSS_COMPILE}readelf -a ${ROOTFS}/bin/busybox | grep "Shared library" |  grep -oP "/?[a-zA-Z0-9_-]+(/[a-zA-Z0-9_-]+)*\.so\.[0-9]+")
toolchain_libraries=${TOOLCHAIN_ROOTFS}/lib64


echo "Copying interpreter $interpreter from ${TOOLCHAIN_ROOTFS} to ${ROOTFS}"
cp ${TOOLCHAIN_ROOTFS}/$interpreter ${ROOTFS}/lib

## Copy the libraries
echo "Copying libraries $libraries from $toolchain_libraries to ${ROOTFS}"
for l in $libraries; do
    cp $toolchain_libraries/$l ${ROOTFS}/lib64
done

# Make device nodes
echo "Creating device nodes"
#cant do this on a mounted device
#sudo mknod -m 666 ${ROOTFS}/dev/null c 1 3
#sudo mknod -m 622 ${ROOTFS}/dev/console c 5 1


# TODO: Clean and build the writer utility

make -C ${FINDER_APP_DIR} clean
make -C ${FINDER_APP_DIR} CROSS_COMPILE=${CROSS_COMPILE} ARCH=${ARCH}

# TODO: Copy the finder related scripts and executables to the /home directory
# on the target rootfs
cp ${FINDER_APP_DIR}/autorun-qemu.sh ${ROOTFS}/home
cp ${FINDER_APP_DIR}/writer ${ROOTFS}/home
cp ${FINDER_APP_DIR}/writer.sh ${ROOTFS}/home
cp ${FINDER_APP_DIR}/finder.sh ${ROOTFS}/home
cp ${FINDER_APP_DIR}/finder-test.sh ${ROOTFS}/home
cp ${FINDER_APP_DIR}/conf/*.txt ${ROOTFS}/home/conf

# TODO: Chown the root directory
sudo chown -R root:root ${ROOTFS}

# TODO: Create initramfs.cpio.gz
#cd "$OUTDIR/rootfs"
#find . | cpio -H newc -o --owner root:root | gzip > ${OUTDIR}/initramfs.cpio.gz

fakeroot bash -c '
  mknod -m 666 ${ROOTFS}/dev/null c 1 3
    mknod -m 622 ${ROOTFS}/dev/console c 5 1
      cd ${ROOTFS}
        find . | cpio -H newc -ov --owner root:root | gzip > ../initramfs.cpio.gz
	'

