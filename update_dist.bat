CD dist
RMDIR /S /Q .\resources
XCOPY ..\resources\ .\resources\ /S /E
XCOPY ..\out\build\Release\game.exe .\ /Y
XCOPY ..\out\build\Release\OpenAL32.dll .\ /Y
XCOPY ..\out\build\Release\SDL2.dll .\ /Y
CD ..