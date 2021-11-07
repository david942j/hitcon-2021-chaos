all: test_ioctl

%: %.c
	$(CC) -static $^ -o $@ -I../linux/drivers/misc/chaos/
