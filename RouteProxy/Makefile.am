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

route_proxy_sources = \
	$(top_srcdir)/RouteProxy/main.cpp \
	$(top_srcdir)/RouteProxy/service_init.cpp \
	$(top_srcdir)/RouteProxy/logic/route_proxy.cpp 

bin_PROGRAMS = route_proxy

route_proxy_SOURCES = $(route_proxy_sources)
route_proxy_LDADD = -lapie -lmysqlclient -lprotobuf -lyaml-cpp
route_proxy_LDFLAGS = $(apie_module_ldflags)