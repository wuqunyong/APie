AM_CXXFLAGS = -Wall -std=c++17

INCLUDES = -I /usr/local/include \
			-I /usr/include/mysql \
			-I /root/yaml-cpp-master/include \
			-I /root/protobuf-3.11.4/src \
			-I $(top_srcdir)/ThirdParty/http-parser \
			-I $(top_srcdir)/ThirdParty/influxdb-cpp \
			-I $(top_srcdir)/PBMsg \
			-I $(top_srcdir)/LibAPie
			
apie_module_ldflags = -L/usr/lib64/mysql -L/usr/local/lib64 -L/usr/local/lib -L/usr/local/protobuf/lib -L$(top_srcdir)/bin -Wl,-rpath,/usr/local/apie/lib -levent -levent_pthreads -lpthread -ldl -lc -lrt -lcrypto -lssl

gateway_server_sources = \
	$(top_srcdir)/GatewayServer/main.cpp \
	$(top_srcdir)/GatewayServer/service_init.cpp \
	$(top_srcdir)/GatewayServer/logic/gateway_mgr.cpp \
	$(top_srcdir)/GatewayServer/logic/model_user.cpp

bin_PROGRAMS = gateway_server

gateway_server_SOURCES = $(gateway_server_sources)
gateway_server_LDADD = -lapie -lmysqlclient -lprotobuf -lyaml-cpp
gateway_server_LDFLAGS = $(apie_module_ldflags)