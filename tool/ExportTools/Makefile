WORK_DIRECTORY   = ./work/
EXPORT_DIRECTORY = ./export/
SVN_URL = svn://svn/mtl/onikiri2/trunk/

DOXYGEN_STRIP_FROM_PATH=$(shell pwd)

.PHONY: all
all: clean base benchmark lib doxygen


.PHONY: base
base: dir
	rm -rf $(WORK_DIRECTORY)/base
	svn export $(SVN_URL)/project $(WORK_DIRECTORY)/base/export/project
	svn export $(SVN_URL)/tool    $(WORK_DIRECTORY)/base/export/tool
	svn export $(SVN_URL)/src     $(WORK_DIRECTORY)/base/work/src
	perl script/licence.pl $(WORK_DIRECTORY)/base/work/src $(WORK_DIRECTORY)/base/export/src/
	(cd $(WORK_DIRECTORY)/base/export; tar -zcf ../../../$(EXPORT_DIRECTORY)/onikiri2-base.tar.gz .;)
	perl script/addrev.pl $(SVN_URL) $(EXPORT_DIRECTORY)/onikiri2-base.tar.gz

.PHONY: benchmark
benchmark: dir
	rm -rf $(WORK_DIRECTORY)/benchmark
	svn export $(SVN_URL)/benchmark   $(WORK_DIRECTORY)/benchmark/benchmark
	(cd $(WORK_DIRECTORY)/benchmark; tar -zcf ../../$(EXPORT_DIRECTORY)/onikiri2-benchmark-spec-bin.tar.gz .;)
	perl script/addrev.pl $(SVN_URL) $(EXPORT_DIRECTORY)/onikiri2-benchmark-spec-bin.tar.gz
	rm -rf $(WORK_DIRECTORY)/benchmark/benchmark/SPECCPU2000_FP/alpha64/bin/*
	rm -rf $(WORK_DIRECTORY)/benchmark/benchmark/SPECCPU2000_INT/alpha64/bin/*
	rm -rf $(WORK_DIRECTORY)/benchmark/benchmark/SPECCPU2000_FP/ppc64/bin/*
	rm -rf $(WORK_DIRECTORY)/benchmark/benchmark/SPECCPU2000_INT/ppc64/bin/*
	rm -rf $(WORK_DIRECTORY)/benchmark/benchmark/SPECCPU2006_FP/alpha64/bin/*
	rm -rf $(WORK_DIRECTORY)/benchmark/benchmark/SPECCPU2006_INT/alpha64/bin/*
	rm -rf $(WORK_DIRECTORY)/benchmark/benchmark/SPECCPU2006_FP/ppc64/bin/*
	rm -rf $(WORK_DIRECTORY)/benchmark/benchmark/SPECCPU2006_INT/ppc64/bin/*
	(cd $(WORK_DIRECTORY)/benchmark; tar -zcf ../../$(EXPORT_DIRECTORY)/onikiri2-benchmark.tar.gz .;)
	perl script/addrev.pl $(SVN_URL) $(EXPORT_DIRECTORY)/onikiri2-benchmark.tar.gz

.PHONY: lib
lib: dir
	rm -rf $(WORK_DIRECTORY)/lib
	svn export $(SVN_URL)/lib   $(WORK_DIRECTORY)/lib/lib
	(cd $(WORK_DIRECTORY)/lib; tar -zcf ../../$(EXPORT_DIRECTORY)/onikiri2-lib.tar.gz .;)
	perl script/addrev.pl $(SVN_URL) $(EXPORT_DIRECTORY)/onikiri2-lib.tar.gz

.PHONY: doxygen
doxygen: dir base lib
	rm -rf $(WORK_DIRECTORY)/doxygen
	mkdir $(WORK_DIRECTORY)/doxygen -p
	cp script/Doxyfile $(WORK_DIRECTORY)/doxygen/Doxyfile
	cp $(WORK_DIRECTORY)/base/export/* $(WORK_DIRECTORY)/doxygen/ -r
	cp $(WORK_DIRECTORY)/lib/* $(WORK_DIRECTORY)/doxygen/ -r
	(cd $(WORK_DIRECTORY)/doxygen/; doxygen ;)
	(cd $(WORK_DIRECTORY)/doxygen/html; tar -zcf ../../../$(EXPORT_DIRECTORY)/onikiri2-doxygen.tar.gz .;)
	perl script/addrev.pl $(SVN_URL) $(EXPORT_DIRECTORY)/onikiri2-doxygen.tar.gz

.PHONY: clean
clean:
	rm $(EXPORT_DIRECTORY)/* -rf
	rm $(WORK_DIRECTORY)/* -rf

.PHONY: dir
dir: 
	mkdir $(WORK_DIRECTORY) -p
	mkdir $(EXPORT_DIRECTORY) -p

