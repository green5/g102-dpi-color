rem unzip hidapi-win.zip -d hidapi/
rem call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\Common7\Tools"\VsDevCmd.bat -arch=x64 >NUL
rem rc main.rc
rem cl /I H:\xbin\include /Zc:__cplusplus /D "_CONSOLE" /EHsc main.cpp main.res gdi32.lib user32.lib /Feg102-dpi-color.exe hidapi\x64\hidapi.lib
rem echo %0: Exit %ERRORLEVEL%
del /S /F /Q build
cmake -Bbuild -A x64 .

