# 
# onikiri2をビルドするMakefile
#

VCPROJ2MAKE      = ../../tool/vcproj2make/version_selecter.pl
VCPROJ2MAKE_DEPS = \
	../../tool/vcproj2make/version_selecter.pl \
	../../tool/vcproj2make/vcxproj2make.pl \
	../../tool/vcproj2make/vcproj2make.pl 

ONIKIRI_MAKE_DIR = onikiri2
ONIKIRI_MAKE_CFG = $(ONIKIRI_MAKE_DIR)/MakeCfg.xml
ONIKIRI_MAKEFILE = $(ONIKIRI_MAKE_DIR)/Makefile

XML2CHEADER_MAKE_DIR = XML2CHeader
XML2CHEADER_MAKE_CFG = $(XML2CHEADER_MAKE_DIR)/MakeCfg.xml
XML2CHEADER_MAKEFILE = $(XML2CHEADER_MAKE_DIR)/Makefile
XML2CHEADER_XML_FILE     = ../../src/DefaultParam.xml
XML2CHEADER_HEADER_FILE  = ../../src/DefaultParam.h
XML2CHEADER_VARIABLE_NAME = g_defaultParam

ZLIB_SRC_DIR  = ../../lib/zlib
ZLIB_MAKE_DIR = zlib
ZLIB_MAKEFILE = $(ZLIB_MAKE_DIR)/Makefile

all : zlib boost xml2cheader onikiri
.PHONY : onikiri zlib boost xml2cheader clean

clean : 
	make -C $(ONIKIRI_MAKE_DIR) clean
	make -C $(ZLIB_MAKE_DIR) clean
	make -C $(XML2CHEADER_MAKE_DIR) clean

#
# --- onikiri
#
onikiri : $(ONIKIRI_MAKEFILE) zlib boost $(XML2CHEADER_HEADER_FILE)
	$(MAKE) -C $(ONIKIRI_MAKE_DIR)

$(ONIKIRI_MAKEFILE) : $(VCPROJ2MAKE_DEPS) $(ONIKIRI_MAKE_CFG)
	perl $(VCPROJ2MAKE) $(ONIKIRI_MAKE_CFG)
	make -C $(ONIKIRI_MAKE_DIR) clean

#
# --- zlib
#

zlib : $(ZLIB_MAKE_DIR)/Makefile
	$(MAKE) -C $(ZLIB_MAKE_DIR)

$(ZLIB_MAKEFILE) : $(ZLIB_MAKE_DIR)/configure
	cd $(ZLIB_MAKE_DIR) ; ./configure

# .svn以下はコピーしない．"."や".."も除外
NON_SVN_PATTERN = -not -path "*/.svn*" -not -name ".*"

# configureがzconf.hをconfigureした環境用に書き換えてしまうので
# Windowsでコンパイルできなくならないようにソースをコピーする
$(ZLIB_MAKE_DIR)/configure : $(ZLIB_SRC_DIR)/zlib.h
	cd $(ZLIB_SRC_DIR); find ./ $(NON_SVN_PATTERN) -type d -print0 | xargs -0 -i mkdir -p $(CURDIR)/$(ZLIB_MAKE_DIR)/{}
	cd $(ZLIB_SRC_DIR); find ./ $(NON_SVN_PATTERN) -type f -print0 | xargs -0 -i cp -f {} $(CURDIR)/$(ZLIB_MAKE_DIR)/{}
	chmod u+x $(ZLIB_MAKE_DIR)/configure

#
# --- boost アーカイブの展開
#
boost : zlib
	$(MAKE) -C ./boost


#
# --- XML2CHeader
#
xml2cheader : $(XML2CHEADER_MAKEFILE)
	$(MAKE) -C $(XML2CHEADER_MAKE_DIR)

$(XML2CHEADER_HEADER_FILE) : $(XML2CHEADER_XML_FILE) xml2cheader
	$(XML2CHEADER_MAKE_DIR)/XML2CHeader $(XML2CHEADER_XML_FILE) $(XML2CHEADER_HEADER_FILE) $(XML2CHEADER_VARIABLE_NAME)

$(XML2CHEADER_MAKEFILE) : $(VCPROJ2MAKE_DEPS) $(XML2CHEADER_MAKE_CFG)
	perl $(VCPROJ2MAKE) $(XML2CHEADER_MAKE_CFG)
	make -C $(XML2CHEADER_MAKE_DIR) clean





