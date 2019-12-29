# RV32UD
# バグってる（TEST_CASE マクロが test_macros じゃなくて aw_test_macros のものを想定）：
#   move structural
# リファレンスがない
#   fcvt_w
# リファレンスがおかしい
#   ldst
# NaN と例外フラグの対応が不完全
#   fcvt fdiv fmin fadd fcmp fmadd
ARC_NAME = rv32ud
SRC_APPS = \
	fclass            \
	recoding 

SRC_DIR = $(RISCV_COMPLIANCE_PATH)/riscv-test-suite/$(ARC_NAME)/src
REF_DIR = $(RISCV_COMPLIANCE_PATH)/riscv-test-suite/$(ARC_NAME)/references
BIN_DIR = ./tmp/work/$(ARC_NAME)
RESULT_DIR = ./tmp/sig/$(ARC_NAME)
ARC_BITS = 32

include TestBody.mk
