@echo off
set curdir=%cd%

set var=DBProxy.exe GatewayServer.exe RouteProxy.exe SceneServer.exe ServiceRegistry.exe
for %%a in (%var%) do (
	tasklist /nh | findstr /i %%a > NUL  
	if ErrorLevel 0 (
	  taskkill /f /t /im "%%a"
	) else (
	  echo not exist %%a
	)
)

pause