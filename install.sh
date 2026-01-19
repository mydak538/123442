#!/bin/bash

# Скрипт установки Python и снятия системных ограничений
# Устанавливает: python3, pip, gcc, clang
# Снимает ограничения Python от системы

set -e  # Прерывать выполнение при ошибках

# Цвета для вывода
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Функции для вывода
info() { echo -e "${BLUE}[INFO]${NC} $1"; }
success() { echo -e "${GREEN}[SUCCESS]${NC} $1"; }
warning() { echo -e "${YELLOW}[WARNING]${NC} $1"; }
error() { echo -e "${RED}[ERROR]${NC} $1"; exit 1; }

# Проверка прав root
check_root() {
    if [ "$EUID" -ne 0 ]; then
        error "Запустите скрипт с sudo: sudo ./install.sh"
    fi
}

# Обновление системы
update_system() {
    info "Обновление системы..."
    apt-get update && apt-get upgrade -y
    success "Система обновлена"
}

# Установка Python и инструментов
install_python_tools() {
    info "Установка Python и инструментов разработки..."
    
    # Основные пакеты
    apt-get install -y \
        python3 \
        python3-pip \
        python3-venv \
        python3-dev \
        python3-wheel \
        python3-setuptools
    
    # Компиляторы и инструменты сборки
    apt-get install -y \
        gcc \
        g++ \
        clang \
        make \
        cmake \
        build-essential
    
    # Дополнительные инструменты для Python
    apt-get install -y \
        python3-full \
        python3-distutils \
        python3-apt \
        python3-venv
    
    success "Python и инструменты установлены"
}

# Снятие системных ограничений с Python
remove_python_restrictions() {
    info "Снятие системных ограничений Python..."
    
    # 1. Настройка PIP для установки в пользовательскую директорию
    mkdir -p /etc/pip.conf 2>/dev/null || true
    
    # Создание конфигурации pip для установки пакетов без ограничений
    cat > /etc/pip.conf << 'EOF'
[global]
break-system-packages = true
EOF
    
    # Альтернативный метод: конфигурация через переменные окружения
    echo 'export PIP_BREAK_SYSTEM_PACKAGES=1' >> /etc/profile.d/python_unrestricted.sh
    
    # 2. Создание альтернативных ссылок для python и pip
    update-alternatives --install /usr/bin/python python /usr/bin/python3 10
    update-alternatives --install /usr/bin/pip pip /usr/bin/pip3 10
    
    # 3. Настройка прав для установки пакетов
    mkdir -p /usr/local/lib/python3.*/dist-packages 2>/dev/null || true
    chmod -R 775 /usr/local/lib/python3.*/dist-packages 2>/dev/null || true
    
    # 4. Отключение защиты от перезаписи системных пакетов
    if [ -f /usr/lib/python3.*/EXTERNALLY-MANAGED ]; then
        mv /usr/lib/python3.*/EXTERNALLY-MANAGED /usr/lib/python3.*/EXTERNALLY-MANAGED.bak
        success "Защита EXTERNALLY-MANAGED отключена"
    fi
    
    # 5. Создание виртуального окружения по умолчанию
    python3 -m venv --system-site-packages /opt/global_venv 2>/dev/null || true
    
    # 6. Настройка PATH для виртуального окружения
    echo 'export PATH="/opt/global_venv/bin:$PATH"' >> /etc/profile.d/python_global_venv.sh
    
    success "Системные ограничения Python сняты"
}

# Настройка политик APT для Python
configure_apt_policies() {
    info "Настройка политик APT для Python..."
    
    # Создание конфигурации для APT
    cat > /etc/apt/apt.conf.d/99python-unrestricted << 'EOF'
# Разрешить установку Python пакетов поверх системных
APT::Get::Assume-Yes "true";
APT::Install-Recommends "true";
APT::Install-Suggests "false";
EOF
    
    # Настройка приоритетов пакетов
    cat > /etc/apt/preferences.d/python-priority << 'EOF'
Package: python3*
Pin: release *
Pin-Priority: 1000
EOF
    
    success "Политики APT настроены"
}

# Установка дополнительных Python инструментов
install_python_extras() {
    info "Установка дополнительных Python инструментов..."
    
    # Обновление pip до последней версии
    python3 -m pip install --upgrade pip --break-system-packages
    
    # Установка основных утилит
    python3 -m pip install --upgrade \
        setuptools \
        wheel \
        virtualenv \
        pipx \
        --break-system-packages
    
    # Настройка pipx для глобальной установки утилит
    python3 -m pipx ensurepath --global 2>/dev/null || true
    
    success "Дополнительные инструменты установлены"
}

# Проверка установки
verify_installation() {
    info "Проверка установки..."
    
    echo "=== Версии установленных компонентов ==="
    python3 --version
    pip3 --version
    gcc --version | head -1
    clang --version | head -1
    
    echo -e "\n=== Проверка возможности установки пакетов ==="
    if python3 -c "import pip; print('PIP работает корректно')"; then
        success "Python настроен правильно"
    else
        warning "Возможны проблемы с Python окружением"
    fi
    
    # Тест установки пакета без ограничений
    echo -e "\n=== Тест установки пакета ==="
    if python3 -m pip install --upgrade numpy --break-system-packages 2>/dev/null; then
        success "Установка пакетов работает без ограничений"
    else
        warning "Для установки некоторых пакетов может потребоваться виртуальное окружение"
    fi
}

# Создание документации
create_documentation() {
    info "Создание документации..."
    
    cat > /usr/local/share/python_unrestricted_README.txt << 'EOF'
УСТАНОВЛЕН БЕЗОПАСНЫЙ РЕЖИМ PYTHON БЕЗ СИСТЕМНЫХ ОГРАНИЧЕНИЙ

Что было сделано:
1. Установлены: python3, pip, gcc, clang, build-essential
2. Сняты системные ограничения на установку пакетов
3. Настроен PIP для установки пакетов поверх системных
4. Создано глобальное виртуальное окружение в /opt/global_venv
5. Отключена защита EXTERNALLY-MANAGED

ВАЖНЫЕ КОМАНДЫ:
- Установка пакетов: pip install <пакет> --break-system-packages
- Обновление pip: python3 -m pip install --upgrade pip --break-system-packages
- Использование глобального venv: source /opt/global_venv/bin/activate

ПРЕДУПРЕЖДЕНИЕ:
Установка пакетов поверх системных может нарушить работу системы.
Рекомендуется использовать виртуальные окружения для проектов:
    python3 -m venv myproject_venv
    source myproject_venv/bin/activate
EOF
    
    success "Документация создана в /usr/local/share/python_unrestricted_README.txt"
}

# Основная функция
main() {
    echo "========================================="
    echo "  Установка Python без системных ограничений  "
    echo "========================================="
    
    check_root
    update_system
    install_python_tools
    remove_python_restrictions
    configure_apt_policies
    install_python_extras
    verify_installation
    create_documentation
    
    echo -e "\n${GREEN}=========================================${NC}"
    echo -e "${GREEN}УСТАНОВКА ЗАВЕРШЕНА УСПЕШНО!${NC}"
    echo -e "${GREEN}=========================================${NC}"
    echo ""
    echo "Перезагрузите терминал или выполните:"
    echo "  source /etc/profile"
    echo ""
    echo "Для установки пакетов используйте флаг:"
    echo "  pip install <пакет> --break-system-packages"
    echo ""
    echo "Или используйте глобальное виртуальное окружение:"
    echo "  source /opt/global_venv/bin/activate"
}

# Запуск основной функции
main
