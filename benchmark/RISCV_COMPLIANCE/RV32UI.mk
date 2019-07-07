# RV32UI
# 入力ファイル
# リファレンスが存在しない & ソースがバグってる？
#	lwu sraw subw ld addiw srliw addw srlw sd sllw sraiw
# バグってる RV_COMPLIANCE_CODE_END がぬけてる
# 	auipc
# 今の鬼斬では対応不能
#   fence_i
# リファレンス出力が多分おかしい
#   sll の 21
#   srli, srl の 4 (32bit の出力になっている)
ARC_NAME = rv32ui
SRC_APPS = \
	andi    bltu     lbu     simple  slti \
	add     bne      or      sltiu   sw \
	addi    beq      lh      ori  slli    sltu      xor \
	bge     jal      lhu     sb      sra      xori \
	bgeu    jalr     lui     srai    \
	and     blt      lb      lw   sh   slt       sub 

SRC_DIR = $(RISCV_COMPLIANCE_PATH)/riscv-test-suite/$(ARC_NAME)/src
REF_DIR = $(RISCV_COMPLIANCE_PATH)/riscv-test-suite/$(ARC_NAME)/references
BIN_DIR = ./tmp/work/$(ARC_NAME)
RESULT_DIR = ./tmp/sig/$(ARC_NAME)
ARC_BITS = 32

include TestBody.mk
