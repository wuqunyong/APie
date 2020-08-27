AM_CXXFLAGS = -Wall -std=c++17

INCLUDES = -I /usr/local/include/ \
			-I /usr/include/mysql/ \
			-I /root/yaml-cpp-master/include \
			-I /root/protobuf-3.11.4/src \
			-I $(top_srcdir)/ThirdParty/http-parser \
			-I $(top_srcdir)/ThirdParty/influxdb-cpp \
			-I $(top_srcdir)/PBMsg \
			-I $(top_srcdir)/LibAPie
			
apie_module_ldflags = -L/usr/lib64/mysql/ -L/usr/local/lib -L/root/yaml-cpp-master/build -L$(top_srcdir)/bin -Wl,-rpath,/usr/local/apie/lib -levent -levent_pthreads -lpthread -ldl -lc -lrt -lcrypto -lssl

service_registry_sources = \
	$(top_srcdir)/ServiceRegistry/main.cpp \
	$(top_srcdir)/ServiceRegistry/service_init.cpp \
	$(top_srcdir)/ServiceRegistry/logic/service_registry.cpp 

bin_PROGRAMS = service_registry

service_registry_SOURCES = $(service_registry_sources)
service_registry_LDADD = -lapie -lmysqlclient -lprotobuf -lyaml-cpp
service_registry_LDFLAGS = $(apie_module_ldflags)