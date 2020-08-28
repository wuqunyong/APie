AM_CXXFLAGS = -Wall -std=c++17

INCLUDES = -I /usr/local/include/ \
			-I /usr/include/mysql/ \
			-I /root/yaml-cpp-master/include \
			-I /root/protobuf-3.11.4/src \
			-I $(top_srcdir)/ThirdParty/http-parser \
			-I $(top_srcdir)/ThirdParty/influxdb-cpp \
			-I $(top_srcdir)/PBMsg \
			-I $(top_srcdir)/LibAPie
			
apie_module_ldflags = -L/usr/lib64/mysql/ -L/usr/local/lib -L/usr/local/protobuf/lib/ -L/root/yaml-cpp-master/build -L$(top_srcdir)/bin -Wl,-rpath,/usr/local/apie/lib -levent -levent_pthreads -lpthread -ldl -lc -lrt -lcrypto -lssl

scene_server_sources = \
	$(top_srcdir)/SceneServer/main.cpp \
	$(top_srcdir)/SceneServer/service_init.cpp \
	$(top_srcdir)/SceneServer/logic/scene_mgr.cpp 

bin_PROGRAMS = scene_server

scene_server_SOURCES = $(scene_server_sources)
scene_server_LDADD = -lapie -lmysqlclient -lprotobuf -lyaml-cpp
scene_server_LDFLAGS = $(apie_module_ldflags)