'Usage:
' From a command line, change directories to the location
' where this script is saved.
' At the command line type: cscript vbdir.vbs [directory]
' ex. cscript shortpath.vbs "C:\Program Files"
' or cscript shortpath.vbs C:\Windows\System32

on error resume next
Set fso=CreateObject("Scripting.FileSystemObject")

Set objFolder = fso.GetFolder(WScript.Arguments(0))

WScript.Echo objFolder.ShortPath
