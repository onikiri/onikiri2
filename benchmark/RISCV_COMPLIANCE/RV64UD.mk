# RV64UD
# バグってる（TEST_CASE マクロが test_macros じゃなくて aw_test_macros のものを想定）：
#   move structural
# リファレンスがない
#   fcvt_w
# リファレンスがおかしい
#   ldst
# NaN と例外フラグの対応が不完全
#   fcvt fdiv fmin fadd fcmp
RV64UD_SRC_APPS = \
	fclass            \
	fmadd recoding 

RV64UD_SRC_DIR = $(RISCV_COMPLIANCE_PATH)/riscv-test-suite/rv32ud/rv64ud
RV64UD_REF_DIR = $(RISCV_COMPLIANCE_PATH)/riscv-test-suite/rv32ud/references
RV64UD_BIN_DIR = ./tmp/work/rv64ud
RV64UD_RESULT_DIR = ./tmp/sig/rv64ud


SRC_APPS = $(RV64UD_SRC_APPS)
SRC_DIR =  $(RV64UD_SRC_DIR)
REF_DIR =  $(RV64UD_REF_DIR)
BIN_DIR =  $(RV64UD_BIN_DIR)
RESULT_DIR =  $(RV64UD_RESULT_DIR)

include TestBody.mk
