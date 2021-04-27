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

route_proxy_sources = \
	$(top_srcdir)/RouteProxy/main.cpp \
	$(top_srcdir)/RouteProxy/service_init.cpp \
	$(top_srcdir)/RouteProxy/logic/route_client.cpp \
	$(top_srcdir)/RouteProxy/logic/route_proxy.cpp 

bin_PROGRAMS = route_proxy

route_proxy_SOURCES = $(route_proxy_sources)
route_proxy_LDADD = -lapie -lmysqlclient -lprotobuf -lyaml-cpp
route_proxy_LDFLAGS = $(apie_module_ldflags)