CD dist
RMDIR /S /Q .\resources
XCOPY ..\resources\ .\resources\ /S /E
XCOPY ..\out\build\Release\base_defense.exe .\ /Y
XCOPY ..\out\build\Release\OpenAL32.dll .\ /Y
XCOPY ..\out\build\Release\SDL2.dll .\ /Y
git add .
git commit -m "updating dist with most recent Release"
CD ..