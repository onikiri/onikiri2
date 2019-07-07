#
# make -f RV64I test-ADDIW のように，各 makefile 内で個別のテストも可能
#

all: all-64 all-32

all-64: test-RV64I test-RV64IM test-RV64UA test-RV64UD test-RV64UF test-RV64UI

all-32: test-RV32I test-RV32IM test-RV32UA test-RV32UD test-RV32UF test-RV32UI

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


test-RV32I:
	$(MAKE) -f RV32I.mk

test-RV32IM:
	$(MAKE) -f RV32IM.mk

test-RV32UA:
	$(MAKE) -f RV32UA.mk

test-RV32UD:
	$(MAKE) -f RV32UD.mk

test-RV32UF:
	$(MAKE) -f RV32UF.mk

test-RV32UI:
	$(MAKE) -f RV32UI.mk
