@echo off
set curdir=%cd%

set var=LoginServer.exe GatewayServer.exe SceneServer.exe RouteProxy.exe ServiceRegistry.exe DBProxy.exe 
for %%a in (%var%) do (
	tasklist /nh | findstr /i %%a > NUL  
	if ErrorLevel 0 (
	  taskkill /f /t /im "%%a"
	) else (
	  echo not exist %%a
	)
)

pause