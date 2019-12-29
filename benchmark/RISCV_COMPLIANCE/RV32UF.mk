# RV32UF
# バグってる（TEST_CASE マクロが test_macros じゃなくて aw_test_macros のものを想定）：
#   ldst
# 演算の例外フラグに対応していないので，現在はテストできない
#	fcvt	fdiv    fmin  fadd    fcmp    fcvt_w  fmadd
ARC_NAME = rv32uf
SRC_APPS = \
	fclass      move \
	recoding

SRC_DIR = $(RISCV_COMPLIANCE_PATH)/riscv-test-suite/$(ARC_NAME)/src
REF_DIR = $(RISCV_COMPLIANCE_PATH)/riscv-test-suite/$(ARC_NAME)/references
BIN_DIR = ./tmp/work/$(ARC_NAME)
RESULT_DIR = ./tmp/sig/$(ARC_NAME)
ARC_BITS = 32

include TestBody.mk
