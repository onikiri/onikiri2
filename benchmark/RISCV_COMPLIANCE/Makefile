# git の管理から除外するために，
# tmp 以下に全てをインストールする
INSTALL_PATH = ./tmp
RISCV_COMPLIANCE_PATH = $(INSTALL_PATH)/riscv-compliance

CC = /home/shioya/opt/gcc/riscv64-linux/7.1.1/bin/riscv64-unknown-linux-gnu-gcc

.DEFAULT_GOAL = all

#
# --- Clone riscv-compliance
#

# checkout riscv-compliane
$(RISCV_COMPLIANCE_PATH): 
	mkdir $(INSTALL_PATH) -p
	cd $(INSTALL_PATH) ;\
		git clone https://github.com/riscv/riscv-compliance

distclean:
	rm $(INSTALL_PATH) -r -f


#
# --- Build binaries
#

# 入力ファイル
SRC_APPS = \
	andi    bltu     lbu     simple  slti \
	add     bne      or      sltiu   sw \
	addi    beq      fence_i lh      ori  slli    sltu      xor \
	bge     jal      lhu     sb      sra      xori \
	bgeu    jalr     lui     srai    \
	and     blt      lb      lw   sh   slt       sub \
	sll     srl      srli \
	
# 
# リファレンスが存在しない & ソースがバグってる？
#	lwu sraw　subw ld addiw srliw　addw　srlw sd sllw sraiw
# バグってる RV_COMPLIANCE_CODE_END がぬけてる
# auipc

#	andi    bltu     lbu  lwu  simple  slti   sraw   subw \
	add     auipc  bne      ld   or   sll     sltiu  srl    sw \
	addi    beq    fence_i  lh   ori  slli    sltu   srli   xor \
	addiw   bge    jal      lhu  sb   slliw   sra    srliw  xori \
	addw    bgeu   jalr     lui  sd   sllw    srai   srlw \
	and     blt    lb       lw   sh   slt     sraiw  sub \

SRC_DIR = $(RISCV_COMPLIANCE_PATH)/riscv-test-suite/rv32ui/rv64ui
REF_DIR = $(RISCV_COMPLIANCE_PATH)/riscv-test-suite/rv32ui/references
BIN_DIR = ./tmp/work/rv64ui
RESULT_FILES_DIR = ./tmp/sig/rv64ui

BIN_FILES = $(SRC_APPS:%=$(BIN_DIR)/%)
RESULT_FILES = $(SRC_APPS:%=$(RESULT_FILES_DIR)/%)
TEST_GOALS = $(SRC_APPS:%=test-%)

# リファレンス出力

TARGET_DIR = ./target-onikiri2
XCFLAGS = -g -static -I $(TARGET_DIR) -I $(RISCV_COMPLIANCE_PATH)/riscv-test-env


all: $(RISCV_COMPLIANCE_PATH)
	$(MAKE) build

# ビルド
build: $(BIN_FILES) 

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(BIN_DIR)/%: $(SRC_DIR)/%.S $(TARGET_DIR)/* Makefile $(BIN_DIR)
	$(CC) $(XCFLAGS) -o $@ $< $(TARGET_DIR)/main.c


# テスト
test: build
	rm $(RESULT_FILES_DIR) -r -f
	$(MAKE) $(RESULT_FILES)
	@echo "==== Test Successful (test-riscv-compliance) ===="

$(RESULT_FILES_DIR):
	mkdir -p $(RESULT_FILES_DIR)

$(RESULT_FILES_DIR)/%: $(RESULT_FILES_DIR)
	@../../project/gcc/onikiri2/a.out param.xml \
		-x /Session/Emulator/Processes/Process/@Command=$(BIN_DIR)/$(notdir $@) \
		-x /Session/Emulator/Processes/Process/@STDOUT=$@ \
		> $@.xml
	@diff $(REF_DIR)/$(notdir $@).reference_output $@; 
	@echo Check $(notdir $@) ... OK

.PHONY: $(TEST_GOALS)
$(TEST_GOALS): 
	$(MAKE) $(BIN_DIR)/$(patsubst test-%,%,$@)
	rm $(RESULT_FILES_DIR)/$(patsubst test-%,%,$@)
	$(MAKE) $(RESULT_FILES_DIR)/$(patsubst test-%,%,$@)

clean:
	rm $(BIN_DIR) -f -r
	rm $(RESULT_FILES_DIR) -r -f
