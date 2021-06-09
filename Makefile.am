SUBDIRS = libapie/Makefile server/db_proxy_server/Makefile server/gateway_server/Makefile server/login_server/Makefile server/route_proxy_server/Makefile server/scene_server/Makefile server/service_registry_server/Makefile server/test_server/Makefile

name=@PACKAGE_NAME@
version=@PACKAGE_VERSION@
release=1
RPMDIR = $(CURDIR)/build
SOURCEDIR = $(CURDIR)

rpm: dist $(name).spec
	@mkdir -p $(RPMDIR)
	rpmbuild -bb --clean --define='_topdir $(RPMDIR)' \
                --define='_rpmdir $(RPMDIR)'		\
                --define='_builddir $(RPMDIR)'		\
                --define='_sourcedir $(SOURCEDIR)'	\
                --define='_specdir $(RPMDIR)'		\
                --define='_srcrpmdir $(RPMDIR)'		\
                $(name).spec

$(name).spec : $(name).spec.in FORCE
	@sed -e 's/@@name@@/$(name)/g' \
	-e 's/@@version@@/$(version)/g' \
	-e 's/@@release@@/$(release)/g' \
	$< > $@

EXTRA_DIST =  \
	conf \
	data \
	etc \
	libapie \
	pb_msg \
	server/common \
	server/db_proxy_server \
	server/gateway_server \
	server/login_server \
	server/route_proxy_server \
	server/scene_server \
	server/service_registry_server \
	server/test_server \
	third_party/astar \
	third_party/http-parser \
	third_party/JPSPlusGoalBounding \
	third_party/influxdb-cpp \
	third_party/jsoncpp \
	third_party/tinyxml2 \
	$(name).spec.in \
	bootstrap.sh

dist-hook:
	echo $(distdir)

FORCE :

genZip: rpm
	$(CURDIR)/make_release.sh

genZipAll: rpm
	$(CURDIR)/make_release.sh base


.PHONY : FORCE
