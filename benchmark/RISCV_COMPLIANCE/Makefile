#
# make -f RV64I test-ADDIW のように，各 makefile 内で個別のテストも可能
#

all: test-RV64I test-RV64IM test-RV64UA test-RV64UD test-RV64UF test-RV64UI

test-RV64I:
	$(MAKE) -f RV64I.mk

test-RV64IM:
	$(MAKE) -f RV64IM.mk

test-RV64UA:
	$(MAKE) -f RV64UA.mk

test-RV64UD:
	$(MAKE) -f RV64UD.mk

test-RV64UF:
	$(MAKE) -f RV64UF.mk

test-RV64UI:
	$(MAKE) -f RV64UI.mk

