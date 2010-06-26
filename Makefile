

all: build/Makefile
	cd build && make

build/Makefile: premake4.lua
	premake4 gmake

clean:
	cd build && make clean

pristine:
	rm -rf build bin lib


