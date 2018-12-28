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
	andi bltu lbu 

#	andi    bltu     lbu  lwu  simple  slti   sraw   subw \
	add     auipc  bne      ld   or   sll     sltiu  srl    sw \
	addi    beq    fence_i  lh   ori  slli    sltu   srli   xor \
	addiw   bge    jal      lhu  sb   slliw   sra    srliw  xori \
	addw    bgeu   jalr     lui  sd   sllw    srai   srlw \
	and     blt    lb       lw   sh   slt     sraiw  sub \

SRC_DIR = $(RISCV_COMPLIANCE_PATH)/riscv-test-suite/rv32ui/rv64ui
REF_DIR = $(RISCV_COMPLIANCE_PATH)/riscv-test-suite/rv32ui/references
DST_DIR = ./tmp/work/rv64ui
RESULTS_DIR = ./tmp/sig/rv64ui

DSTS = $(SRC_APPS:%=$(DST_DIR)/%)
RESULTS = $(SRC_APPS:%=$(RESULTS_DIR)/%)

# リファレンス出力

TARGET = ./target-onikiri2
XCFLAGS = -g -static -I $(TARGET) -I $(RISCV_COMPLIANCE_PATH)/riscv-test-env


all: $(RISCV_COMPLIANCE_PATH)
	mkdir -p $(DST_DIR)
	$(MAKE) build

# ビルド
build: $(DSTS) 

$(DST_DIR)/%: $(SRC_DIR)/%.S $(TARGET)/* Makefile
	$(CC) $(XCFLAGS) -o $@ $< $(TARGET)/main.c


# テスト
test: build
	rm $(RESULTS_DIR) -r
	mkdir -p $(RESULTS_DIR)
	$(MAKE) $(RESULTS)

$(RESULTS_DIR)/%: 
	@../../project/gcc/onikiri2/a.out param.xml \
		-x /Session/Emulator/Processes/Process/@Command=$(DST_DIR)/$(notdir $@) \
		-x /Session/Emulator/Processes/Process/@STDOUT=$@ \
		> $@.xml
	@diff $(REF_DIR)/$(notdir $@).reference_output $@; 
	@echo Check $(notdir $@) ... OK


