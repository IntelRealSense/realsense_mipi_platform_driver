RM := rm -rf
PKG := d4xx-build
PKGENV := d4xx-env
REPO := local
PWD := $(shell pwd)
BUILD_DATE := $(shell date +"%Y-%m-%-d-%H-%M-%S")
BUILD_DEST := ${PWD}/out/${BUILD_DATE}
VERSION := $(shell grep 'LABEL version' Dockerfile.env | awk -F'=' '{print $$2}' | tr -d '\"')

GIT_REPO_TAG := $(shell git describe --abbrev=0 --tags 2>/dev/null)
GIT_REPO_ROOT := $(shell git rev-parse --show-toplevel 2>/dev/null)

ifeq ($(GIT_REPO_TAG), )
	GIT_REPO_TAG := latest
endif

ifeq ($(GIT_REPO_ROOT), )
	GIT_REPO_ROOT := ./
endif

all: env build
docker: env

env:
	docker build -t $(REPO)/$(PKGENV):$(VERSION) -f Dockerfile.env ./
	docker tag $(REPO)/$(PKGENV):$(VERSION) $(REPO)/$(PKGENV):latest

build:
	docker build -t $(REPO)/$(PKG):$(VERSION) \
		--build-arg GIT_REPO_ROOT=$(GIT_REPO_ROOT) \
		-f Dockerfile.build $(GIT_REPO_ROOT)
	mkdir -p ${BUILD_DEST}
	docker run --rm -it -v ${BUILD_DEST}:/out/ \
	$(REPO)/$(PKG):$(VERSION) \
	/bin/bash -c 'cp images/arch/arm64/boot/Image \
		images/drivers/media/i2c/d4xx.ko \
		images/arch/arm64/boot/dts/tegra194-p2888-0001-p2822-0000.dtb \
		modules.tar.xz /out/; \
		chown -R --reference=/out /out'

push:

tag:
	docker tag $(REPO)$(PKG):$(VERSION) $(REPO)/$(PKG):latest

pack:
	docker save -o ./$(PKG)_$(VERSION).tar $(REPO)/$(PKG):$(VERSION)
	gzip -vf $(PKG)_$(VERSION).tar
	echo "Use on target: 'docker load -i $(PKG)_$(VERSION).tar'"

run:
	docker run --rm -it $(REPO)/$(PKG):$(VERSION)

clean-out:
	${RM} out/* 

clean-env:
	docker rmi $(REPO)/$(PKGENV):latest $(REPO)/$(PKGENV):$(VERSION)

clean-build:
	docker rmi $(REPO)/$(PKG):$(VERSION)

clean: clean-env clean-build clean-out
	-@echo ' '

clean-all: clean
	
.PHONY: all clean
.SECONDARY:

