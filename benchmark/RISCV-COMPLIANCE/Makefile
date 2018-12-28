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
SRC_TESTS = \
    add \
	addi \

SRC_DIR = $(RISCV_COMPLIANCE_PATH)/riscv-test-suite/rv32ui/rv64ui
SRCS = $(SRC_TESTS:%=$(SRC_DIR)/%.S)

# 出力
DST_DIR = ./work/rv64ui
DSTS = $(SRC_TESTS:%=$(DST_DIR)/%)

# リファレンス出力
REF_DIR = $(RISCV_COMPLIANCE_PATH)/riscv-test-suite/rv32ui/references

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
test:
	../../project/gcc/onikiri2/a.out	
