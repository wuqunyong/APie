
call :startfunc TestServer.exe ../../conf/test_client.yaml

goto end

:startfunc
	start %1 %2
	
goto :eof

:end