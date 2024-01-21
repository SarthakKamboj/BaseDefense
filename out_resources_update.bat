RMDIR /S /Q .\out\build\Debug\resources
RMDIR /S /Q .\out\build\Release\resources
XCOPY .\resources\ .\out\build\Debug\resources\ /S /E
XCOPY .\resources\ .\out\build\Release\resources\ /S /E