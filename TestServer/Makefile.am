AM_CXXFLAGS = -Wall -std=c++17

INCLUDES = -I /usr/local/include \
			-I /usr/include/mysql \
			-I /root/yaml-cpp-master/include \
			-I /root/protobuf-3.11.4/src \
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

test_client_sources = \
	$(top_srcdir)/TestServer/test_client.cpp \
	$(top_srcdir)/TestServer/service_init.cpp \
	$(top_srcdir)/TestServer/logic/mock_role.cpp \
	$(top_srcdir)/TestServer/logic/test_server.cpp 


bin_PROGRAMS = test_client

test_client_SOURCES = $(test_client_sources)
test_client_LDADD = -lapie -lmysqlclient -lprotobuf -lyaml-cpp
test_client_LDFLAGS = $(apie_module_ldflags)