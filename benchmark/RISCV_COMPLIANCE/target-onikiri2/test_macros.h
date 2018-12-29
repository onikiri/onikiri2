#include "aw_test_macros.h"
#include "compliance_test.h"

// "_onikiri2_begin_output_data" must be aligned
#define RVTEST_DATA_BEGIN \
    .align 4 ;\
    _onikiri2_begin_output_data:

// "_onikiri2_end_output_data" also must be aligned
#define RVTEST_DATA_END \
    .align 4; \
    _onikiri2_end_output_data:

