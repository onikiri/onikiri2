# RV64I
RV64I_SRC_APPS = \
	ADDIW  ADDW  SLLIW  SLLW  SRAIW  SRAW  SRLIW  SRLW  SUBW 
RV64I_SRC_DIR = $(RISCV_COMPLIANCE_PATH)/riscv-test-suite/rv64i/src
RV64I_REF_DIR = $(RISCV_COMPLIANCE_PATH)/riscv-test-suite/rv64i/references
RV64I_BIN_DIR = ./tmp/work/rv64i
RV64I_RESULT_DIR = ./tmp/sig/rv64i


SRC_APPS = $(RV64I_SRC_APPS)
SRC_DIR =  $(RV64I_SRC_DIR)
REF_DIR =  $(RV64I_REF_DIR)
BIN_DIR =  $(RV64I_BIN_DIR)
RESULT_DIR =  $(RV64I_RESULT_DIR)
ARC_BITS = 64

include TestBody.mk
