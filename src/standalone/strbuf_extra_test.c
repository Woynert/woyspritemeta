#include "strbuf_extra.h"
#include "woytest.h"


TEST test_general(void) {
    const strview_t invalid = ((strview_t){.data=NULL,.size=0});
    ASSERT(strview_ends_with(cstr_SL("Hello there"), cstr_SL("there")));
    ASSERT(strview_ends_with(cstr_SL(""), cstr_SL("")));
    ASSERT(strview_ends_with(invalid, cstr_SL("")));
    ASSERT(strview_ends_with(cstr_SL(""), invalid));
    ASSERT(strview_ends_with(invalid, invalid));
    ASSERT_FALSE(strview_ends_with(invalid, cstr_SL("mimo")));
    ASSERT_TRUE(strview_ends_with(cstr_SL("myfilesare.here.txt"), cstr_SL(".here.txt")));
    ASSERT_TRUE(strview_ends_with(cstr_SL("0123456789"), cstr_SL("123456789")));
    ASSERT_TRUE(strview_ends_with(cstr_SL("123456789"), cstr_SL("123456789")));
    ASSERT_TRUE(strview_ends_with(cstr_SL("\n\n\n"), cstr_SL("\n")));
    TEST_PASS;
}


int main(void) {
    TESTS_INIT();
    RUN_TEST(test_general);
    TESTS_SHOW_RESULTS();
}
