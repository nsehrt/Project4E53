@ECHO OFF
xcopy "Release\Output\Project4E53.exe" /Y
upx.exe "Project4E53.exe"