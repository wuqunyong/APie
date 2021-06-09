AM_CXXFLAGS = -Wall -std=c++17

INCLUDES = -I /usr/local/include \
			-I /usr/include/mysql \
			-I /usr/local/protobuf/include \
			-I $(top_srcdir)/ThirdParty/astar \
			-I $(top_srcdir)/ThirdParty/cpp_redis/includes \
			-I $(top_srcdir)/ThirdParty/cpp_redis/tacopie/includes \
			-I $(top_srcdir)/ThirdParty/http-parser \
			-I $(top_srcdir)/ThirdParty/jsoncpp/include \
			-I $(top_srcdir)/ThirdParty/tinyxml2 \
			-I $(top_srcdir)/ThirdParty/influxdb-cpp \
			-I $(top_srcdir)/PBMsg \
			-I $(top_srcdir)/LibAPie \
			-I $(top_srcdir)/SharedDir
			
apie_module_ldflags = -L/usr/lib64/mysql -L/usr/local/lib64 -L/usr/local/lib -L/usr/local/protobuf/lib -L$(top_srcdir)/bin -Wl,-rpath,/usr/local/apie/lib -levent -levent_pthreads -lpthread -ldl -lc -lrt -lcrypto -lssl -llz4 -lcpp_redis -ltacopie

pb_sources = \
	$(top_srcdir)/PBMsg/BusinessMsg/login_msg.pb.cc \
	$(top_srcdir)/PBMsg/BusinessMsg/rpc_login.pb.cc

shared_dir_sources = 

gateway_server_sources = \
	$(top_srcdir)/GatewayServer/main.cpp \
	$(top_srcdir)/GatewayServer/service_init.cpp \
	$(top_srcdir)/GatewayServer/logic/gateway_mgr.cpp \
	$(top_srcdir)/GatewayServer/logic/gateway_role.cpp

bin_PROGRAMS = gateway_server

gateway_server_SOURCES = $(gateway_server_sources) $(pb_sources) $(shared_dir_sources)
gateway_server_LDADD = -lapie -lmysqlclient -lprotobuf -lyaml-cpp
gateway_server_LDFLAGS = $(apie_module_ldflags)