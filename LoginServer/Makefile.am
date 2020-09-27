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

login_server_sources = \
	$(top_srcdir)/LoginServer/main.cpp \
	$(top_srcdir)/LoginServer/service_init.cpp \
	$(top_srcdir)/LoginServer/logic/login_mgr.cpp 

bin_PROGRAMS = login_server

scene_server_SOURCES = $(login_server_sources)
scene_server_LDADD = -lapie -lmysqlclient -lprotobuf -lyaml-cpp
scene_server_LDFLAGS = $(apie_module_ldflags)