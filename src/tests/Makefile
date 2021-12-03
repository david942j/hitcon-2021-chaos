UAPI_HEADER_DIR = ../linux/drivers/misc/chaos
UAPI_HEADER = $(UAPI_HEADER_DIR)/chaos.h
all: test_ioctl

%: %.c $(UAPI_HEADER)
	$(CC) -static $< -o $@ -I$(UAPI_HEADER_DIR)
	strip -s $@
