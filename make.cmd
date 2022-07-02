unzip hidapi-win.zip -d hidapi/
call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\Common7\Tools"\VsDevCmd.bat -arch=x64 >NUL
rc main.rc
cl /I H:\xbin\include /Zc:__cplusplus /D "_CONSOLE" /EHsc main.cpp main.res gdi32.lib user32.lib /Feg102-dpi-color.exe hidapi\x64\hidapi.lib
echo %0: Exit %ERRORLEVEL%
