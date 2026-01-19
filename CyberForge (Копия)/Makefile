# Компилятор и флаги
CC = gcc
CFLAGS = -m32 -nostdlib -nostdinc -ffreestanding -Wall -I./kernel/include -fno-pic -O1
LD = ld
LDFLAGS = -m elf_i386 -T boot/linker.ld -nostdlib

# Объектные файлы ядра (добавили vga.o)
KERNEL_OBJS = kernel/kernel.o \
              kernel/drivers/screen.o \
              kernel/drivers/keyboard.o \
              kernel/drivers/ports.o \
              kernel/drivers/vga.o \
              kernel/gui/gui.o \
              kernel/fs/fs.o \
              kernel/shell/commands.o \
              kernel/lib/string.o \
              kernel/lib/stdlib.o

# Цели по умолчанию
all: bin/kernel.bin bin/wexos.iso

# Сборка ядра
bin/kernel.bin: $(KERNEL_OBJS)
	@mkdir -p bin
	$(LD) $(LDFLAGS) -o $@ $(KERNEL_OBJS)
	@echo "✓ Kernel built: bin/kernel.bin"
	@if [ -f "$@" ]; then \
		echo "  Size: $$(stat -c%s $@ 2>/dev/null || echo "unknown") bytes"; \
	else \
		echo "  Error: Kernel binary not created"; \
	fi

# Правила компиляции
kernel/%.o: kernel/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Создание заголовочных файлов
kernel/include/ports.h:
	@mkdir -p kernel/include
	@echo "Creating ports.h..."
	@echo '#ifndef PORTS_H' > $@
	@echo '#define PORTS_H' >> $@
	@echo '' >> $@
	@echo '#include "types.h"' >> $@
	@echo '' >> $@
	@echo '/* Порт ввода/вывода */' >> $@
	@echo 'static inline u8 inb(u16 port) {' >> $@
	@echo '    u8 data;' >> $@
	@echo '    asm volatile("inb %%dx, %%al" : "=a"(data) : "d"(port));' >> $@
	@echo '    return data;' >> $@
	@echo '}' >> $@
	@echo '' >> $@
	@echo 'static inline void outb(u16 port, u8 data) {' >> $@
	@echo '    asm volatile("outb %%al, %%dx" : : "a"(data), "d"(port));' >> $@
	@echo '}' >> $@
	@echo '' >> $@
	@echo 'static inline u16 inw(u16 port) {' >> $@
	@echo '    u16 data;' >> $@
	@echo '    asm volatile("inw %%dx, %%ax" : "=a"(data) : "d"(port));' >> $@
	@echo '    return data;' >> $@
	@echo '}' >> $@
	@echo '' >> $@
	@echo 'static inline void outw(u16 port, u16 data) {' >> $@
	@echo '    asm volatile("outw %%ax, %%dx" : : "a"(data), "d"(port));' >> $@
	@echo '}' >> $@
	@echo '' >> $@
	@echo '#endif' >> $@

kernel/include/vga.h:
	@mkdir -p kernel/include
	@echo "Creating vga.h..."
	@echo '#ifndef VGA_H' > $@
	@echo '#define VGA_H' >> $@
	@echo '' >> $@
	@echo 'void vga_set_mode_13h(void);' >> $@
	@echo 'void vga_set_mode_text(void);' >> $@
	@echo 'void vga_put_pixel(int x, int y, unsigned char color);' >> $@
	@echo 'void vga_clear_screen(unsigned char color);' >> $@
	@echo 'void vga_draw_rect(int x, int y, int width, int height, unsigned char color);' >> $@
	@echo '' >> $@
	@echo '#endif' >> $@

# Сборка ISO
bin/wexos.iso: bin/kernel.bin
	@echo "Сборка ISO образа..."
	@mkdir -p iso/boot/grub
	
	# Копируем ядро
	@cp bin/kernel.bin iso/boot/
	
	# Создаем минимальный grub.cfg
	@echo 'set timeout=0' > iso/boot/grub/grub.cfg
	@echo 'set default=0' >> iso/boot/grub/grub.cfg
	@echo '' >> iso/boot/grub/grub.cfg
	@echo 'menuentry "CyberForge OS" {' >> iso/boot/grub/grub.cfg
	@echo '    multiboot /boot/kernel.bin' >> iso/boot/grub/grub.cfg
	@echo '    boot' >> iso/boot/grub/grub.cfg
	@echo '}' >> iso/boot/grub/grub.cfg
	
	# Пробуем разные способы создания ISO
	@if command -v grub-mkrescue > /dev/null 2>&1; then \
		echo "Используется grub-mkrescue..."; \
		grub-mkrescue -o bin/wexos.iso iso/ 2>/dev/null || \
		grub-mkrescue -o bin/wexos.iso iso/; \
	elif command -v grub2-mkrescue > /dev/null 2>&1; then \
		echo "Используется grub2-mkrescue..."; \
		grub2-mkrescue -o bin/wexos.iso iso/; \
	elif command -v xorriso > /dev/null 2>&1; then \
		echo "Используется xorriso..."; \
		xorriso -as mkisofs -R -b boot/grub/i386-pc/eltorito.img \
		        -no-emul-boot -boot-load-size 4 -boot-info-table \
		        -o bin/wexos.iso iso/; \
	else \
		echo "Ошибка: Установите grub-mkrescue или xorriso"; \
		echo "  Ubuntu/Debian: sudo apt install grub-pc-bin xorriso"; \
		echo "  Fedora/RHEL: sudo dnf install grub2-tools xorriso"; \
		echo "  Arch: sudo pacman -S grub xorriso"; \
		false; \
	fi
	
	@if [ -f "bin/wexos.iso" ]; then \
		echo "✓ ISO built: bin/wexos.iso"; \
		echo "  Size: $$(stat -c%s bin/wexos.iso 2>/dev/null || echo "unknown") bytes"; \
	else \
		echo "✗ Ошибка создания ISO образа"; \
	fi
	
	@rm -rf iso/

# Очистка
clean:
	rm -f $(KERNEL_OBJS) bin/kernel.bin bin/wexos.iso

# Полная очистка
distclean: clean
	rm -rf bin/ kernel/include/ports.h kernel/include/vga.h

# Запуск через ISO (графический режим)
iso-run: bin/wexos.iso
	@echo "Запуск через ISO образ (графический режим)..."
	@if [ -f "bin/wexos.iso" ]; then \
		qemu-system-i386 -cdrom bin/wexos.iso -m 512 -serial stdio -vga std; \
	else \
		echo "Ошибка: bin/wexos.iso не найден"; \
		echo "Выполните 'make all' сначала"; \
	fi

# Запуск через ISO с curses
iso-curses: bin/wexos.iso
	@echo "Запуск ISO в curses режиме..."
	@if [ -f "bin/wexos.iso" ]; then \
		qemu-system-i386 -cdrom bin/wexos.iso -m 512 -display curses; \
	else \
		echo "Ошибка: bin/wexos.iso не найден"; \
		echo "Выполните 'make all' сначала"; \
	fi

# Запуск через kernel напрямую (графический)
run: bin/kernel.bin
	@echo "Запуск CyberForge OS в графическом режиме..."
	@if [ -f "bin/kernel.bin" ]; then \
		qemu-system-i386 -kernel bin/kernel.bin -m 512 -vga std; \
	else \
		echo "Ошибка: bin/kernel.bin не найден"; \
		echo "Выполните 'make' сначала"; \
	fi

# Запуск через kernel (текстовый режим)
term: bin/kernel.bin
	@echo "Запуск в терминале..."
	@if [ -f "bin/kernel.bin" ]; then \
		qemu-system-i386 -kernel bin/kernel.bin -nographic -serial mon:stdio -m 512; \
	else \
		echo "Ошибка: bin/kernel.bin не найден"; \
		echo "Выполните 'make' сначала"; \
	fi

# Отладка
debug: bin/kernel.bin
	@echo "Запуск в режиме отладки..."
	@if [ -f "bin/kernel.bin" ]; then \
		qemu-system-i386 -kernel bin/kernel.bin -monitor stdio -serial stdio -m 512 -s -S -vga std; \
	else \
		echo "Ошибка: bin/kernel.bin не найден"; \
		echo "Выполните 'make' сначала"; \
	fi

# Быстрая сборка и запуск ISO
quick-iso: bin/wexos.iso iso-run

# Цель для создания всех необходимых заголовочных файлов
headers: kernel/include/ports.h kernel/include/vga.h
	@echo "✓ Заголовочные файлы созданы"

# Цель для сборки всех компонентов по отдельности
build-all: headers $(KERNEL_OBJS) bin/kernel.bin bin/wexos.iso
	@echo "✓ Все компоненты собраны"

# Информация о системе
info:
	@echo "CyberForge OS Build System"
	@echo "=========================="
	@echo "CC: $(CC)"
	@echo "CFLAGS: $(CFLAGS)"
	@echo "Kernel objects:"
	@for obj in $(KERNEL_OBJS); do \
		if [ -f "$$obj" ]; then \
			echo "  ✓ $$obj"; \
		else \
			echo "  ✗ $$obj (missing)"; \
		fi; \
	done
	@if [ -f "bin/kernel.bin" ]; then \
		echo "✓ Kernel: bin/kernel.bin"; \
	else \
		echo "✗ Kernel: not built"; \
	fi
	@if [ -f "bin/wexos.iso" ]; then \
		echo "✓ ISO: bin/wexos.iso"; \
	else \
		echo "✗ ISO: not built"; \
	fi

# Тест графики
test-graphics: bin/kernel.bin
	@echo "Тест графического режима..."
	@if [ -f "bin/kernel.bin" ]; then \
		qemu-system-i386 -kernel bin/kernel.bin -m 512 -vga std -full-screen; \
	else \
		echo "Ошибка: bin/kernel.bin не найден"; \
	fi

.PHONY: all clean distclean run term debug iso-run iso-curses quick-iso headers build-all info test-graphics
