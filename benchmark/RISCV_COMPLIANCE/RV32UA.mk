# RV32UA
ARC_NAME = rv32ua
SRC_APPS = \
	amoand_w   amomaxu_w  amominu_w  amoswap_w \
	amoadd_w  amomax_w   amomin_w   amoor_w    amoxor_w \
  	lrsc

SRC_DIR = $(RISCV_COMPLIANCE_PATH)/riscv-test-suite/$(ARC_NAME)/src
REF_DIR = $(RISCV_COMPLIANCE_PATH)/riscv-test-suite/$(ARC_NAME)/references
BIN_DIR = ./tmp/work/$(ARC_NAME)
RESULT_DIR = ./tmp/sig/$(ARC_NAME)
ARC_BITS = 32

include TestBody.mk
