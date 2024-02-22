override C_FILES := $(shell find src/ -type f -name '*.c')
override C_OBJS := $(patsubst src/%.c,obj/%.c.o,$(C_FILES))

override CC := clang
override LD := clang

override C_FLAGS := \
	-target x86_64-unknown-windows \
	-ffreestanding \
	-fshort-wchar \
	-mno-red-zone \
	-I inc/ \
	-I ../inc/

override LD_FLAGS := \
	-target x86_64-unknown-windows \
	-nostdlib \
	-Wl,-entry:efi_main \
	-Wl,-subsystem:efi_application \
	-fuse-ld=lld-link

.PHONY: all clean

all: $(C_OBJS)
	clang $(LD_FLAGS) -o obj/BOOTX64.EFI $^

obj/%.c.o: src/%.c
	mkdir -p $(@D)
	clang $(C_FLAGS) -o $@ -c $<

clean:
	rm -rf obj