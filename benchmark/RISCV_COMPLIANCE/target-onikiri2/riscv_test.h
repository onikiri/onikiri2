//
// 主要な機能は compliance_test.h 側に実装
// こちらのヘッダは，古いベンチマークだと読み込まれる？
//

#ifndef ONIKIRI2_RISCV_TEST_H
#define ONIKIRI2_RISCV_TEST_H


#define RVTEST_RV64U
#define RVTEST_RV64UF

// エントリポイント
// main.c から関数として呼ばれる
// 全レジスタを保存する
#define RVTEST_CODE_BEGIN \
    .text ;\
    .align 4 ;\
    .global onikiri2_test_main ;\
    onikiri2_test_main: ;\
        ONIKIRI2_SAVE_REGS() ;\

// 終了
// スタックポインタを含むレジスタが破壊されているので，復元し，
// 検証用のシグニチャをダンプした後，main.c に返る
#define RVTEST_CODE_END \
    onikiri2_test_main_end: ; \
        ONIKIRI2_RESTORE_REGS() ;\
        ONIKIRI2_SAVE_REGS() ;\
        la a0, _onikiri2_begin_output_data ;\
        la a1, _onikiri2_end_output_data ;  \
        jal onikiri2_rvtest_print_result ;  \
        ONIKIRI2_RESTORE_REGS() ;\
        ret ;


// gp レジスタに，その時実行中のテスト番号が格納される
#define TESTNUM gp

// SWSIG(testnum, testreg) は，
// test_res: のメモリの testnum 番目に testreg の値を書き込む
// test_res は RV_COMPLIANCE_DATA_BEGIN にある

// pass: の直後にこのマクロが展開される
#define RVTEST_PASS                     \
    li TESTNUM, 1;                      \
    SWSIG (0, TESTNUM);                 \
    j onikiri2_test_main_end ;

// fail: の直後にこのマクロが展開される
#define RVTEST_FAIL                     \
    sll TESTNUM, TESTNUM, 1;            \
    or TESTNUM, TESTNUM, 1;             \
    SWSIG (0, TESTNUM);                 \
    j onikiri2_test_main_end ;


#endif
