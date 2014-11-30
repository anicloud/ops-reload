.PHONY: all comake check release debug clean install output mrproper test doc

MAJOR_VERSION=2.0.7

SCMPF_MODULE_VERSION=$(MAJOR_VERSION).$(shell svn info Makefile | tail -9 | head -1 | awk '{print $$NF}')

TAR=tar --owner=0 --group=0 --mode=go-w
NAME=matrix
CONTROL=agent-control

output: release debug test
	@rm -rf output
	@mkdir -p output/NOAH output/bin output/etc
	@cp bin/$(NAME) output/bin
	@cp etc/$(NAME).conf output/etc
	@cp bin/$(CONTROL) output/bin
	@cp matrix_jail.sh output/bin
	@cp tools/update_meta.py output/bin
	@cp bin/jail output/bin
	@cp NOAH/* output/NOAH
	@cp -r adapter_scripts output/adapter_scripts
	@find output/adapter_scripts -name .svn | xargs rm -rf
	@chmod 0755 output/bin output/etc output/NOAH output/adapter_scripts
	@chmod 0755 output/bin/*
	@chmod 0644 output/etc/*
	@chmod 0755 output/NOAH/control
	@chmod 4755 output/bin/jail
	@cd output && find -type f -exec md5sum {} \; >> ../md5sums
	@cp bin/$(NAME).syms output
	@cp bin/$(CONTROL).syms output
	@chmod 0644 md5sums
	@mv md5sums output/NOAH/
	@cd output && $(TAR) -czf $(NAME).tgz NOAH bin etc adapter_scripts
	@chmod 0644 output/$(NAME).tgz
	@if [ -n "${SCMPF_MODULE_VERSION}" ]; then cp output/$(NAME).tgz "output/$(NAME)-${SCMPF_MODULE_VERSION}.tgz"; fi

all: release debug

comake:
	@$(MAKE) -C comake

release: lib-release
	@$(MAKE) -C main release
	@$(MAKE) -C control release
	@$(MAKE) -C tools/jail release

debug: lib-debug
	@$(MAKE) -C main debug
	@$(MAKE) -C control debug
	@$(MAKE) -C tools/jail debug

lib-release: mrproper
	@$(MAKE) -C src release

lib-debug: mrproper
	@$(MAKE) -C src debug

clean:
	@$(MAKE) -C test clean
	@$(MAKE) -C main clean
	@$(MAKE) -C src clean
	@$(MAKE) -C control clean
	@$(MAKE) -C tools/jail clean
	@rm -rf revision.c tags output doc bin lib

mrproper:
	@env SCMPF_MODULE_VERSION=$(SCMPF_MODULE_VERSION) ../../../../op/oped/noah/sailor/utility/revision.sh

test: lib-release lib-debug
	@$(MAKE) -C test mrproper
	@$(MAKE) -C test all

check: test
	@$(MAKE) -C test test

doc:
	@-doxygen
