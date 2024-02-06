RMDIR /S /Q .\out\build\Debug\resources
RMDIR /S /Q .\out\build\Release\resources
RMDIR /S /Q .\out\build\RelWithDebInfo\resources
XCOPY .\resources\ .\out\build\Debug\resources\ /S /E
XCOPY .\resources\ .\out\build\Release\resources\ /S /E
XCOPY .\resources\ .\out\build\RelWithDebInfo\resources\ /S /E