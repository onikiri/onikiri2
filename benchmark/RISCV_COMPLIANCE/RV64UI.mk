# RV64UI
# 入力ファイル
# リファレンスが存在しない & ソースがバグってる？
#	lwu sraw　subw ld addiw srliw　addw　srlw sd sllw sraiw
# バグってる RV_COMPLIANCE_CODE_END がぬけてる
# 	auipc
RV64UI_SRC_APPS = \
	andi    bltu     lbu     simple  slti \
	add     bne      or      sltiu   sw \
	addi    beq      fence_i lh      ori  slli    sltu      xor \
	bge     jal      lhu     sb      sra      xori \
	bgeu    jalr     lui     srai    \
	and     blt      lb      lw   sh   slt       sub \
	sll     srl      srli 
RV64UI_SRC_DIR = $(RISCV_COMPLIANCE_PATH)/riscv-test-suite/rv32ui/rv64ui
RV64UI_REF_DIR = $(RISCV_COMPLIANCE_PATH)/riscv-test-suite/rv32ui/references
RV64UI_BIN_DIR = ./tmp/work/rv64ui
RV64UI_RESULT_DIR = ./tmp/sig/rv64ui


SRC_APPS = $(RV64UI_SRC_APPS)
SRC_DIR =  $(RV64UI_SRC_DIR)
REF_DIR =  $(RV64UI_REF_DIR)
BIN_DIR =  $(RV64UI_BIN_DIR)
RESULT_DIR =  $(RV64UI_RESULT_DIR)

include TestBody.mk
