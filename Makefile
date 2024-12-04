KDIR ?= /lib/modules/$(shell uname -r)/build

default:
	make -C $(KDIR) M=$(PWD) modules
.PHONY: all

reload:
	./scripts/reload_module.sh
.PHONY: reload

clean:
	make -C $(KDIR) M=$(PWD) clean
.PHONY: clean
