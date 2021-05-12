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

test_client_sources = \
	$(top_srcdir)/TestServer/test_client.cpp \
	$(top_srcdir)/TestServer/service_init.cpp \
	$(top_srcdir)/TestServer/logic/mock_role.cpp \
	$(top_srcdir)/TestServer/logic/test_case.cpp \
	$(top_srcdir)/TestServer/logic/test_runner.cpp \
	$(top_srcdir)/TestServer/logic/test_server.cpp \
	$(top_srcdir)/TestServer/logic/test_suite.cpp \
	$(top_srcdir)/TestServer/logic/task/echo_test_case.cpp \
	$(top_srcdir)/TestServer/logic/task/login_test_case.cpp \
	$(top_srcdir)/TestServer/logic/task/logou_test_case.cpp

pb_sources = \
	$(top_srcdir)/PBMsg/BusinessMsg/login_msg.pb.cc \
	$(top_srcdir)/PBMsg/BusinessMsg/rpc_login.pb.cc \
	$(top_srcdir)/PBMsg/BusinessMsg/role_server_msg.pb.cc

bin_PROGRAMS = test_client

test_client_SOURCES = $(test_client_sources) $(pb_sources)
test_client_LDADD = -lapie -lmysqlclient -lprotobuf -lyaml-cpp
test_client_LDFLAGS = $(apie_module_ldflags)