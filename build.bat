@echo off
setlocal

REM ============== BUILD CONFIG ========================
set VS_BUILDTOOLS_VERSION=16.4.3
set DEVKIT_PACKAGE_PATH=devkit_package
REM ====================================================

REM ==============  BUILD ORDER ==============
call "%BPCDEV_PATH%\vs_buildtools\%VS_BUILDTOOLS_VERSION%\VC\Auxiliary\Build\vcvarsall.bat" x64_x86
call .\generate_projects.bat
call :CLEAN
call :BUILD
call :PACKAGE
exit 0
REM ==========================================

REM ======================================================== SUBROUTINES ========================================================
goto :EOF

:CLEAN
	echo "Starting clean commands in parallel..."
	(
	start msbuild .\build\x86-debug\libdfu.sln /t:clean /p:Configuration=Debug /p:Platform=Win32
	start msbuild .\build\x86-release\libdfu.sln /t:clean /p:Configuration=Release /p:Platform=Win32
	) | pause
	echo "All done..."
	goto :EOF

:BUILD
	msbuild .\build\x86-debug\libdfu.sln /t:Build /p:Configuration=Debug /p:Platform=Win32
	msbuild .\build\x86-release\libdfu.sln /t:Build /p:Configuration=Release /p:Platform=Win32
	goto :EOF

:PACKAGE
	xcopy .\build\x86-debug\blpdevupd\Debug\blpdevupd.exe .\%DEVKIT_PACKAGE_PATH%\Debug\ /I /Y
	xcopy .\build\x86-debug\blpdevupd\Debug\blpdevupd.pdb .\%DEVKIT_PACKAGE_PATH%\Debug\ /I /Y
	xcopy .\build\x86-debug\blpdevupd\Debug\libusb-1.0.dll .\%DEVKIT_PACKAGE_PATH%\Debug\ /I /Y
	xcopy .\build\x86-debug\dfudll\Debug\libdfu.dll .\%DEVKIT_PACKAGE_PATH%\Debug\ /I /Y
	xcopy .\build\x86-debug\dfudll\Debug\libdfu.pdb .\%DEVKIT_PACKAGE_PATH%\Debug\ /I /Y

	xcopy .\build\x86-release\blpdevupd\Release\blpdevupd.exe .\%DEVKIT_PACKAGE_PATH%\Release\ /I /Y
	xcopy .\build\x86-release\blpdevupd\Release\libusb-1.0.dll .\%DEVKIT_PACKAGE_PATH%\Release\ /I /Y
	xcopy .\build\x86-release\dfudll\Release\libdfu.dll .\%DEVKIT_PACKAGE_PATH%\Release\ /I /Y
	goto :EOF
