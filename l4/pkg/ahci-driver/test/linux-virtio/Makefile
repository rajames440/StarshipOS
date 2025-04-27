PKGDIR ?= ../..
L4DIR  ?= $(PKGDIR)/../..

MODE := l4linux
TEST_MODE := l4linux
SYSTEMS := x86-l4f amd64-l4f

TEST_GROUP    := ahci-driver/linux-virtio

EXTRA_TEST    := virtio_read

TEST_TARGET_virtio_read  := virtio_read.sh

# The test image with GPT, will be created in the test setup script
IMAGE_FILE     = $(OBJ_DIR)/test_ahci.img
TEST_SETUP     = $(SRC_DIR)/create-test-disk.sh $(IMAGE_FILE)

REQUIRED_MODULES := l4linux-ahci.io io ahci-drv
NED_CFG       := l4linux-ahci.cfg

QEMU_ARGS      = -device ahci,id=ahci0 -drive if=none,file=$(IMAGE_FILE),format=raw,id=drive-sata0-0-0 -device ide-hd,bus=ahci0.0,drive=drive-sata0-0-0,id=sata0-0-0
L4LINUX_CONF  := CONFIG_L4_VIRTIO CONFIG_VIRTIO_BLK

include $(L4DIR)/mk/test.mk
