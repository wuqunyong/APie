
call :startfunc DBProxy.exe ../../conf/db_proxy.yaml
call :startfunc GatewayServer.exe ../../conf/gateway_server.yaml
call :startfunc RouteProxy.exe ../../conf/route_proxy.yaml
call :startfunc SceneServer.exe ../../conf/scene_server.yaml
rem call :startfunc ServiceRegistry.exe ../../conf/service_registry.yaml

goto end

:startfunc
	tasklist /nh | findstr /i %1 > NUL  
	if ErrorLevel 1 (
	  start %1 %2
	) else (
	  echo exist %1 %2
	)
	
goto :eof

:end