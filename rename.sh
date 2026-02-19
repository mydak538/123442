sudo bash << 'EOF'
#!/bin/bash

echo "🔄 Изменяю систему на CyberForge GNU/Linux..."

# 1. Изменяем /etc/os-release (для neofetch и системы)
cat > /etc/os-release << 'OSFILE'
NAME="CyberForge"
VERSION="1.0 (Cyber Edition)"
ID=cyberforge
ID_LIKE=ubuntu
PRETTY_NAME="CyberForge GNU/Linux 1.0"
VERSION_ID="1.0"
HOME_URL="https://cyberforge.local"
SUPPORT_URL="https://cyberforge.local/support"
BUG_REPORT_URL="https://cyberforge.local/bugs"
UBUNTU_CODENAME=noble
LOGO=cyberforge-logo
OSFILE

# 2. Изменяем GRUB (для меню загрузки)
sed -i 's/^GRUB_DISTRIBUTOR=.*/GRUB_DISTRIBUTOR="CyberForge"/' /etc/default/grub 2>/dev/null || \
echo 'GRUB_DISTRIBUTOR="CyberForge"' >> /etc/default/grub

# 3. Обновляем GRUB
update-grub 2>/dev/null || grub-mkconfig -o /boot/grub/grub.cfg

# 4. Меняем hostname (имя компьютера)
echo "cyberforge-pc" > /etc/hostname
hostnamectl set-hostname cyberforge-pc

# 5. Обновляем /etc/hosts
sed -i 's/127.0.1.1\s.*/127.0.1.1\tcyberforge-pc/' /etc/hosts

# 6. Создаем логотип для neofetch (по желанию)
mkdir -p /usr/share/cyberforge
cat > /usr/share/cyberforge/logo.txt << 'LOGO'
    ╔══════════════════════════════╗
    ║     CYBERFORGE GNU/LINUX     ║
    ╚══════════════════════════════╝
LOGO

echo "✅ Готово! Система теперь CyberForge GNU/Linux"
EOF
