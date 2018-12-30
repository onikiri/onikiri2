# git の管理から除外するために，
# tmp 以下に全てをインストールする
WORK_PATH = ./tmp
RISCV_COMPLIANCE_PATH = $(WORK_PATH)/riscv-compliance

CC = /home/shioya/opt/gcc/riscv64-linux/7.1.1/bin/riscv64-unknown-linux-gnu-gcc

.DEFAULT_GOAL = all

#
# --- Clone riscv-compliance
#

# checkout riscv-compliane
$(RISCV_COMPLIANCE_PATH): 
	mkdir $(WORK_PATH) -p
	cd $(WORK_PATH) ;\
		git clone https://github.com/riscv/riscv-compliance

distclean:
	rm $(WORK_PATH) -r -f


#
# --- Build binaries
#

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

# RV64UF
# バグってる（TEST_CASE マクロが test_macros じゃなくて aw_test_macros を使ってる）：
#   ldst
RV64UF_SRC_APPS = \
	fclass  fcvt    fdiv    fmin  move \
	fadd    fcmp    fcvt_w  fmadd recoding
RV64UF_SRC_DIR = $(RISCV_COMPLIANCE_PATH)/riscv-test-suite/rv32uf/rv64uf
RV64UF_REF_DIR = $(RISCV_COMPLIANCE_PATH)/riscv-test-suite/rv32uf/references
RV64UF_BIN_DIR = ./tmp/work/rv64uf
RV64UF_RESULT_DIR = ./tmp/sig/rv64uf

# RV64I
RV64I_SRC_APPS = \
	ADDIW  ADDW  SLLIW  SLLW  SRAIW  SRAW  SRLIW  SRLW  SUBW 
RV64I_SRC_DIR = $(RISCV_COMPLIANCE_PATH)/riscv-test-suite/rv64i/src
RV64I_REF_DIR = $(RISCV_COMPLIANCE_PATH)/riscv-test-suite/rv64i/references
RV64I_BIN_DIR = ./tmp/work/rv64i
RV64I_RESULT_DIR = ./tmp/sig/rv64i


SRC_APPS = $(RV64UI_SRC_APPS)
SRC_DIR =  $(RV64UI_SRC_DIR)
REF_DIR =  $(RV64UI_REF_DIR)
BIN_DIR =  $(RV64UI_BIN_DIR)
RESULT_DIR =  $(RV64UI_RESULT_DIR)



BIN_FILES    = $(SRC_APPS:%=$(BIN_DIR)/%)
RESULT_FILES = $(SRC_APPS:%=$(RESULT_DIR)/%)
TEST_GOALS   = $(SRC_APPS:%=test-%)

# リファレンス出力

# ビルド設定
TARGET_DIR = ./target-onikiri2
XCFLAGS = -g -static -I $(TARGET_DIR) -I $(RISCV_COMPLIANCE_PATH)/riscv-test-env


all: init
	$(MAKE) test

init: $(RISCV_COMPLIANCE_PATH)

# ビルド
build: $(BIN_FILES) 

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(BIN_DIR)/%: $(SRC_DIR)/%.S $(TARGET_DIR)/* Makefile $(BIN_DIR)
	$(CC) $(XCFLAGS) -o $@ $< $(TARGET_DIR)/main.c


# テスト
test: build
	rm $(RESULT_DIR) -r -f
	$(MAKE) $(RESULT_FILES)
	@echo "==== Test Successful (test-riscv-compliance) ===="

$(RESULT_DIR):
	mkdir -p $(RESULT_DIR)

$(RESULT_DIR)/%: $(RESULT_DIR)
	../../project/gcc/onikiri2/a.out param.xml \
		-x /Session/Emulator/Processes/Process/@Command=$(BIN_DIR)/$(notdir $@) \
		-x /Session/Emulator/Processes/Process/@STDOUT=$@ \
		> $@.xml
	diff $(REF_DIR)/$(notdir $@).reference_output $@; 
	@echo Check $(notdir $@) ... OK

.PHONY: $(TEST_GOALS)
$(TEST_GOALS): 
	$(MAKE) $(BIN_DIR)/$(patsubst test-%,%,$@)
	rm $(RESULT_DIR)/$(patsubst test-%,%,$@) -f
	$(MAKE) $(RESULT_DIR)/$(patsubst test-%,%,$@)

clean:
	rm $(BIN_DIR) -f -r
	rm $(RESULT_DIR) -r -f
