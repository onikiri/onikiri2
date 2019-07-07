# RV32I
RV32I_SRC_APPS = \
	I-ADD-01     I-CSRRSI-01       I-LHU-01            I-SLLI-01 \
	I-ADDI-01    I-CSRRW-01        I-LUI-01            I-SLT-01 \
	I-AND-01     I-CSRRWI-01       I-LW-01             I-SLTI-01 \
	I-ANDI-01    I-DELAY_SLOTS-01  I-MISALIGN_JMP-01   I-SLTIU-01 \
	I-AUIPC-01   I-EBREAK-01       I-MISALIGN_LDST-01  I-SLTU-01 \
	I-BEQ-01     I-ECALL-01        I-NOP-01            I-SRA-01 \
	I-BGE-01     I-ENDIANESS-01    I-OR-01             I-SRAI-01 \
	I-BGEU-01    I-FENCE.I-01      I-ORI-01            I-SRL-01 \
	I-BLT-01     I-IO              I-RF_size-01        I-SRLI-01 \
	I-BLTU-01    I-JAL-01          I-RF_width-01       I-SUB-01 \
	I-BNE-01     I-JALR-01         I-RF_x0-01          I-SW-01 \
	I-CSRRC-01   I-LB-01           I-SB-01             I-XOR-01 \
	I-CSRRCI-01  I-LBU-01          I-SH-01             I-XORI-01 \
	I-CSRRS-01   I-LH-01           I-SLL-01

RV32I_SRC_DIR = $(RISCV_COMPLIANCE_PATH)/riscv-test-suite/rv32i/src
RV32I_REF_DIR = $(RISCV_COMPLIANCE_PATH)/riscv-test-suite/rv32i/references
RV32I_BIN_DIR = ./tmp/work/rv32i
RV32I_RESULT_DIR = ./tmp/sig/rv32i


SRC_APPS = $(RV32I_SRC_APPS)
SRC_DIR =  $(RV32I_SRC_DIR)
REF_DIR =  $(RV32I_REF_DIR)
BIN_DIR =  $(RV32I_BIN_DIR)
RESULT_DIR =  $(RV32I_RESULT_DIR)
ARC_BITS = 32

include TestBody.mk
