

all: build/Makefile
	cd build && make

build/Makefile: premake4.lua
	premake4 gmake

clean:
	cd build && make clean

pristine:
	rm -rf build bin lib

check:
	LC_ALL=C egrep '[^_.>a-zA-Z0-9](str(n?cpy|n?cat|xfrm|n?dup|str|pbrk|tok|_)|stpn?cpy|r?index[^.]|a?sn?printf|byte_)' `find src -name "*.c" -print | egrep -v "(bsafe|bstraux)"`

