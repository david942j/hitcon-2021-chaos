MAKEFILE_DIR=$(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))
PROJ_DIR=$(shell realpath "${MAKEFILE_DIR}/..")
QEMU_DIR=${PROJ_DIR}/qemu
LINUX_DIR=${PROJ_DIR}/linux

copy_qemu_src: qemu qemu.patch
	cd ${QEMU_DIR} && grep chaos hw/misc/meson.build || git apply ${MAKEFILE_DIR}/qemu.patch
	cd qemu && $(RM) -r ${QEMU_DIR}/hw/misc/chaos && ln -s ${MAKEFILE_DIR}/qemu/hw/misc/chaos ${QEMU_DIR}/hw/misc/chaos

driver: linux
	$(MAKE) -C ${LINUX_DIR} M=${MAKEFILE_DIR}/linux/drivers/misc/chaos chaos.ko -j `nproc`
