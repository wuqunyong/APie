AM_CXXFLAGS = -Wall -std=c++17

INCLUDES = -I /usr/local/include \
			-I /usr/include/mysql \
			-I /usr/local/protobuf/include \
			-I $(top_srcdir)/third_party/astar \
			-I $(top_srcdir)/third_party/cpp_redis/includes \
			-I $(top_srcdir)/third_party/cpp_redis/tacopie/includes \
			-I $(top_srcdir)/third_party/http-parser \
			-I $(top_srcdir)/third_party/jsoncpp/include \
			-I $(top_srcdir)/third_party/tinyxml2 \
			-I $(top_srcdir)/third_party/influxdb-cpp \
			-I $(top_srcdir)/pb_msg \
			-I $(top_srcdir)/libapie \
			-I $(top_srcdir)/common
			
apie_module_ldflags = -L/usr/lib64/mysql -L/usr/local/lib64 -L/usr/local/lib -L/usr/local/protobuf/lib -L$(top_srcdir)/bin -Wl,-rpath,/usr/local/apie/lib -levent -levent_pthreads -lpthread -ldl -lc -lrt -lcrypto -lssl -llz4 -lcpp_redis -ltacopie

test_client_sources = \
	$(top_srcdir)/server/test_server/test_client.cpp \
	$(top_srcdir)/server/test_server/service_init.cpp \
	$(top_srcdir)/server/test_server/logic/mock_role.cpp \
	$(top_srcdir)/server/test_server/logic/test_case.cpp \
	$(top_srcdir)/server/test_server/logic/test_runner.cpp \
	$(top_srcdir)/server/test_server/logic/test_server.cpp \
	$(top_srcdir)/server/test_server/logic/test_suite.cpp \
	$(top_srcdir)/server/test_server/logic/task/echo_test_case.cpp \
	$(top_srcdir)/server/test_server/logic/task/login_test_case.cpp \
	$(top_srcdir)/server/test_server/logic/task/logout_test_case.cpp

pb_sources = \
	$(top_srcdir)/pb_msg/business/login_msg.pb.cc \
	$(top_srcdir)/pb_msg/business/rpc_login.pb.cc \
	$(top_srcdir)/pb_msg/business/role_server_msg.pb.cc

bin_PROGRAMS = test_client

test_client_SOURCES = $(test_client_sources) $(pb_sources)
test_client_LDADD = -lapie -lmysqlclient -lprotobuf -lyaml-cpp
test_client_LDFLAGS = $(apie_module_ldflags)