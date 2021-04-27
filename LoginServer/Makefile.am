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

login_server_sources = \
	$(top_srcdir)/LoginServer/main.cpp \
	$(top_srcdir)/LoginServer/service_init.cpp \
	$(top_srcdir)/LoginServer/logic/login_mgr.cpp 

bin_PROGRAMS = login_server

login_server_SOURCES = $(login_server_sources) $(pb_sources)
login_server_LDADD = -lapie -lmysqlclient -lprotobuf -lyaml-cpp
login_server_LDFLAGS = $(apie_module_ldflags)