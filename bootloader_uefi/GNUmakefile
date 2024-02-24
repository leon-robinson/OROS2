override C_FILES := $(shell find src/ -type f -name '*.c')
override C_OBJS := $(patsubst src/%.c,obj/%.c.o,$(C_FILES))
override AS_FILES := $(shell find src/ -type f -name '*.asm')
override AS_OBJS := $(patsubst src/%.asm,obj/%.asm.o,$(AS_FILES))

override CC := clang-18
override LD := clang-18
override AS := nasm

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

override AS_FLAGS := \
	-f bin

.PHONY: all clean link asm

all:
	$(MAKE) asm
	$(MAKE) link

link: $(C_OBJS)
	$(CC) $(LD_FLAGS) -o obj/BOOTX64.EFI $^

asm: $(AS_OBJS)

obj/%.c.o: src/%.c
	mkdir -p $(@D)
	$(LD) $(C_FLAGS) -o $@ -c $<

obj/%.asm.o: src/%.asm
	mkdir -p $(@D)
	$(AS) $(AS_FLAGS) $< -o $@

clean:
	rm -rf obj