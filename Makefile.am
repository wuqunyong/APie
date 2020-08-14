SUBDIRS = bin

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
	bootstrap.sh  \
	$(name).spec.in \
	LibApie


FORCE :

genZip: rpm
	$(CURDIR)/make_release.sh

genZipAll: rpm
	$(CURDIR)/make_release.sh base


.PHONY : FORCE