
call :startfunc TestServer.exe ../../conf/test_client.yaml

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