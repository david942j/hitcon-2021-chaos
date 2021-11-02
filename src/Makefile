MAKEFILE_DIR=$(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))
PROJ_DIR=$(shell realpath "${MAKEFILE_DIR}/..")
QEMU_DIR=${PROJ_DIR}/qemu

copy_qemu_src: qemu qemu.patch
	cd ${QEMU_DIR} && git checkout . && git apply ${MAKEFILE_DIR}/qemu.patch
	cd qemu && cp -r --parents hw/ ${QEMU_DIR}/

