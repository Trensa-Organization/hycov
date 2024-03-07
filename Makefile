NAME = hycov
PREFIX = /usr

all:
	$(MAKE) clear
	$(MAKE) release

release:
	cmake --no-warn-unused-cli -DCMAKE_BUILD_TYPE:STRING=Debug -DLEGACY_RENDERER:BOOL=true -S . -B ./build -G Ninja
	cmake --build ./build
	chmod -R 777 ./build

clear:
	rm -rf build

install.core:
	@if [ ! -f ./build/libhycov.so ]; then echo -en "You need to run $(MAKE) all first.\n" && exit 1; fi
	@echo -en "!NOTE: Please note make install does not compile Hycov and only installs the already built files."
	mkdir -p ${PREFIX}/lib
	cp -f ./build/libhycov.so ${PREFIX}/lib
	 chmod 755 ${PREFIX}/lib/libhycov.so

install: install.core

uninstall:
