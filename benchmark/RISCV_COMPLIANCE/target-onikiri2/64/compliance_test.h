
#ifndef ONIKIRI2_COMPLIANCE_TEST_H
#define ONIKIRI2_COMPLIANCE_TEST_H

#define RV_COMPLIANCE_RV32M


// gp レジスタに，その時実行中のテスト番号が格納される
#define TESTNUM gp


// pass: fail: の直後にこのマクロが展開される
#define RV_COMPLIANCE_HALT \
    li TESTNUM, 1;                      \
    SWSIG (0, TESTNUM);                 \
    j onikiri2_test_main_end ;


// エントリポイント
// main.c から関数として呼ばれる
// 全レジスタを保存する
#define RV_COMPLIANCE_CODE_BEGIN \
    .text ;\
    .align 4 ;\
    .global onikiri2_test_main ;\
    onikiri2_test_main: ;\
        ONIKIRI2_SAVE_REGS() ;\

// 終了
// スタックポインタを含むレジスタが破壊されているので，復元し，
// 検証用のシグニチャをダンプした後，main.c に返る
#define RV_COMPLIANCE_CODE_END \
    onikiri2_test_main_end: ; \
        ONIKIRI2_RESTORE_REGS() ;\
        ONIKIRI2_SAVE_REGS() ;\
        la a0, _onikiri2_begin_output_data ;\
        la a1, _onikiri2_end_output_data ;  \
        jal onikiri2_rvtest_print_result ;  \
        ONIKIRI2_RESTORE_REGS() ;\
        ret ;


// "_onikiri2_begin_output_data" must be aligned
#define RV_COMPLIANCE_DATA_BEGIN \
    .align 4 ;\
    _onikiri2_begin_output_data:

// "_onikiri2_end_output_data" also must be aligned
#define RV_COMPLIANCE_DATA_END \
    .align 4; \
    _onikiri2_end_output_data:

// レジスタ保存
#define ONIKIRI2_SAVE_REGS() \
    addi sp, sp, -8*32 ;\
    sd x0, 0*8(sp) ;\
    sd x1, 1*8(sp) ;\
    sd x2, 2*8(sp) ;\
    sd x3, 3*8(sp) ;\
    sd x4, 4*8(sp) ;\
    sd x5, 5*8(sp) ;\
    sd x6, 6*8(sp) ;\
    sd x7, 7*8(sp) ;\
    sd x8, 8*8(sp) ;\
    sd x9, 9*8(sp) ;\
    sd x10, 10*8(sp) ;\
    sd x11, 11*8(sp) ;\
    sd x12, 12*8(sp) ;\
    sd x13, 13*8(sp) ;\
    sd x14, 14*8(sp) ;\
    sd x15, 15*8(sp) ;\
    sd x16, 16*8(sp) ;\
    sd x17, 17*8(sp) ;\
    sd x18, 18*8(sp) ;\
    sd x19, 19*8(sp) ;\
    sd x20, 20*8(sp) ;\
    sd x21, 21*8(sp) ;\
    sd x22, 22*8(sp) ;\
    sd x23, 23*8(sp) ;\
    sd x24, 24*8(sp) ;\
    sd x25, 25*8(sp) ;\
    sd x26, 26*8(sp) ;\
    sd x27, 27*8(sp) ;\
    sd x28, 28*8(sp) ;\
    sd x29, 29*8(sp) ;\
    sd x30, 30*8(sp) ;\
    sd x31, 31*8(sp) ;\
    la t0, onikiri2_sp_save ;\
    sd sp, 0(t0) ;\

// レジスタ復帰
#define ONIKIRI2_RESTORE_REGS() \
    la t0, onikiri2_sp_save ;\
    ld sp, 0(t0) ;\
    ld x0, 0*8(sp) ;\
    ld x1, 1*8(sp) ;\
    ld x2, 2*8(sp) ;\
    ld x3, 3*8(sp) ;\
    ld x4, 4*8(sp) ;\
    ld x5, 5*8(sp) ;\
    ld x6, 6*8(sp) ;\
    ld x7, 7*8(sp) ;\
    ld x8, 8*8(sp) ;\
    ld x9, 9*8(sp) ;\
    ld x10, 10*8(sp) ;\
    ld x11, 11*8(sp) ;\
    ld x12, 12*8(sp) ;\
    ld x13, 13*8(sp) ;\
    ld x14, 14*8(sp) ;\
    ld x15, 15*8(sp) ;\
    ld x16, 16*8(sp) ;\
    ld x17, 17*8(sp) ;\
    ld x18, 18*8(sp) ;\
    ld x19, 19*8(sp) ;\
    ld x20, 20*8(sp) ;\
    ld x21, 21*8(sp) ;\
    ld x22, 22*8(sp) ;\
    ld x23, 23*8(sp) ;\
    ld x24, 24*8(sp) ;\
    ld x25, 25*8(sp) ;\
    ld x26, 26*8(sp) ;\
    ld x27, 27*8(sp) ;\
    ld x28, 28*8(sp) ;\
    ld x29, 29*8(sp) ;\
    ld x30, 30*8(sp) ;\
    ld x31, 31*8(sp) ;\
    addi sp, sp, 8*32 \




// main.c で定義
// begin から end の範囲をダンプ
// void onikiri2_rvtest_print_result(uint32_t* begin, uint32_t*end)
.extern onikiri2_rvtest_print_result

//
// data/code sections
//

.data
.align 8
// スタックポインタの保存用領域
onikiri2_sp_save:
    .dword 0x0000000000000000


#endif  // RSD_COMPLIANCE_TEST_H
