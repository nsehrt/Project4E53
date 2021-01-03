@ECHO OFF
xcopy "bin\Release\Project4E53.exe" /Y
upx.exe "Project4E53.exe" -9 -q