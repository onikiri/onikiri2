# RV32IM
ARC_NAME = rv32im
SRC_APPS = \
	DIV  DIVU  MUL  MULH  MULHSU  MULHU  REM  REMU

SRC_DIR = $(RISCV_COMPLIANCE_PATH)/riscv-test-suite/$(ARC_NAME)/src
REF_DIR = $(RISCV_COMPLIANCE_PATH)/riscv-test-suite/$(ARC_NAME)/references
BIN_DIR = ./tmp/work/$(ARC_NAME)
RESULT_DIR = ./tmp/sig/$(ARC_NAME)
ARC_BITS = 32

include TestBody.mk
