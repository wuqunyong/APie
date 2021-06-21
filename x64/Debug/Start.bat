
call :startfunc "AccountDBProxy" DBProxy.exe ../../conf/db_account_proxy.yaml
call :startfunc "RoleDBProxy" DBProxy.exe ../../conf/db_role_proxy.yaml
call :startfunc "LoginServer" LoginServer.exe ../../conf/login_server.yaml
call :startfunc "GatewayServer" GatewayServer.exe ../../conf/gateway_server.yaml
rem call :startfunc "RouteProxy" RouteProxy.exe ../../conf/route_proxy.yaml
call :startfunc "SceneServer" SceneServer.exe ../../conf/scene_server.yaml
call :startfunc "ServiceRegistry" ServiceRegistry.exe ../../conf/service_registry.yaml

goto end

:startfunc
	tasklist /nh | findstr /i %1 > NUL  
	if ErrorLevel 1 (
	  start %1 %2 %3
	) else (
	  echo exist %1 %2 %3
	)
	
goto :eof

:end