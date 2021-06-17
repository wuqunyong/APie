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
			
apie_module_ldflags = -L/usr/lib64/mysql -L/usr/local/lib64 -L/usr/local/lib -L/usr/local/protobuf/lib -L$(top_srcdir)/libapie -Wl,-rpath,/usr/local/apie/lib -levent -levent_pthreads -lpthread -ldl -lc -lrt -lcrypto -lssl -llz4 -lcpp_redis -ltacopie -lnats_static

pb_sources = \
	$(top_srcdir)/pb_msg/business/login_msg.pb.cc \
	$(top_srcdir)/pb_msg/business/rpc_login.pb.cc

login_server_sources = \
	$(top_srcdir)/server/login_server/main.cpp \
	$(top_srcdir)/server/login_server/service_init.cpp \
	$(top_srcdir)/server/login_server/logic/login_mgr.cpp 

bin_PROGRAMS = login_server

login_server_SOURCES = $(login_server_sources) $(pb_sources)
login_server_LDADD = -lapie -lmysqlclient -lprotobuf -lyaml-cpp
login_server_LDFLAGS = $(apie_module_ldflags)