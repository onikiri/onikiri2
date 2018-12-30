# RV64UA
# バグってる
#   amoadd_d amomax_d amomin_d amoor_d
#   amoand_d  amomaxu_d  amominu_d  amoswap_d
RV64UA_SRC_APPS = \
	amoand_w   amomaxu_w  amominu_w  amoswap_w \
	amoadd_w  amomax_w   amomin_w   amoor_w    amoxor_w \
  lrsc
RV64UA_SRC_DIR = $(RISCV_COMPLIANCE_PATH)/riscv-test-suite/rv32ua/rv64ua
RV64UA_REF_DIR = $(RISCV_COMPLIANCE_PATH)/riscv-test-suite/rv32ua/references
RV64UA_BIN_DIR = ./tmp/work/rv64ua
RV64UA_RESULT_DIR = ./tmp/sig/rv64ua


SRC_APPS = $(RV64UA_SRC_APPS)
SRC_DIR =  $(RV64UA_SRC_DIR)
REF_DIR =  $(RV64UA_REF_DIR)
BIN_DIR =  $(RV64UA_BIN_DIR)
RESULT_DIR =  $(RV64UA_RESULT_DIR)

include TestBody.mk
