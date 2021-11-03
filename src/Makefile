MAKEFILE_DIR=$(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))
PROJ_DIR=$(shell realpath "${MAKEFILE_DIR}/..")
QEMU_DIR=${PROJ_DIR}/qemu
LINUX_DIR=${PROJ_DIR}/linux

copy_qemu_src: qemu qemu.patch
	cd ${QEMU_DIR} && git checkout . && git apply ${MAKEFILE_DIR}/qemu.patch
	cd qemu && cp -r --parents hw/ ${QEMU_DIR}/

driver: linux
	$(MAKE) -C ${LINUX_DIR} M=${MAKEFILE_DIR}/linux/drivers/misc/chaos chaos.ko -j `nproc`
