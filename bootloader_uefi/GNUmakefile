override C_FILES := $(shell find src/ -type f -name '*.c')
override C_OBJS := $(patsubst src/%.c,obj/%.c.o,$(C_FILES))

override CC := clang-18
override LD := clang-18

override C_FLAGS := \
	-target x86_64-unknown-windows \
	-ffreestanding \
	-fshort-wchar \
	-mno-red-zone \
	-std=c17 \
	-x c \
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
	$(CC) $(LD_FLAGS) -o obj/BOOTX64.EFI $^

obj/%.c.o: src/%.c
	mkdir -p $(@D)
	$(LD) $(C_FLAGS) -o $@ -c $<

clean:
	rm -rf obj