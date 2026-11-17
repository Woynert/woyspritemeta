.PHONY: *
default:

CC ?= gcc

# ===== basic commands =====

clean:
	rm -rf build build_release

compile:
	premake5 ninja --cc=$(CC) && ninja -C build && premake5 compilecommands

release:
	premake5 ninja --cc=$(CC) && ninja -C build Release && premake5 compilecommands

run:
	LSAN_OPTIONS=suppressions=suppr.txt LD_LIBRARY_PATH=/etc/lsb/lib/ ./build/bin/Debug/meta

#note(woy): Must be called from project root.
compile_tcc:
	premake5 ninja --cc=gcc && PATH=$(PWD)/tccalias:$(PATH) time -p ninja -C build && premake5 compilecommands


run_tcc:
	LD_LIBRARY_PATH=/etc/lsb/lib/ ./build/bin/Debug/meta
