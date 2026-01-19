#!/bin/bash
echo "=== Building CyberForge ==="

# Создаем директории
mkdir -p bin
mkdir -p iso/boot/grub

# Компилируем
echo "Compiling kernel..."
gcc -m32 -nostdlib -nostdinc -ffreestanding -Wall -I./kernel/include \
    -c kernel/kernel.c -o kernel/kernel.o

echo "Compiling drivers..."
gcc -m32 -nostdlib -nostdinc -ffreestanding -Wall -I./kernel/include \
    -c kernel/drivers/screen.c -o kernel/drivers/screen.o
gcc -m32 -nostdlib -nostdinc -ffreestanding -Wall -I./kernel/include \
    -c kernel/drivers/keyboard.c -o kernel/drivers/keyboard.o

echo "Compiling libraries..."
gcc -m32 -nostdlib -nostdinc -ffreestanding -Wall -I./kernel/include \
    -c kernel/lib/string.c -o kernel/lib/string.o
gcc -m32 -nostdlib -nostdinc -ffreestanding -Wall -I./kernel/include \
    -c kernel/lib/stdlib.c -o kernel/lib/stdlib.o

echo "Compiling filesystem..."
gcc -m32 -nostdlib -nostdinc -ffreestanding -Wall -I./kernel/include \
    -c kernel/fs/fs.c -o kernel/fs/fs.o

echo "Compiling shell..."
gcc -m32 -nostdlib -nostdinc -ffreestanding -Wall -I./kernel/include \
    -c kernel/shell/commands.c -o kernel/shell/commands.o

# Линковка
echo "Linking..."
ld -m elf_i386 -T boot/linker.ld -nostdlib \
    -o bin/kernel.bin \
    kernel/kernel.o \
    kernel/drivers/screen.o \
    kernel/drivers/keyboard.o \
    kernel/lib/string.o \
    kernel/lib/stdlib.o \
    kernel/fs/fs.o \
    kernel/shell/commands.o

echo "✓ Build complete: bin/kernel.bin"
echo ""
echo "To run: qemu-system-i386 -kernel bin/kernel.bin -serial stdio"
echo "Or: make run"
