#include <sys/ctype.h>
#include <sys/kernel.h>
#include <sys/string.h>
#include <sys/testing.h>

KERNEL_TEST(string_memset_memcpy) {
    char buf[16];
    char src[16];

    ASSERT_EQ(memset(buf, 0xAB, sizeof(buf)), (void *) buf);
    for (size_t i = 0; i < sizeof(buf); i++) { EXPECT_EQ((unsigned char) buf[i], 0xAB); }

    memset(buf, 0, sizeof(buf));
    memset(buf + 4, 'x', 4);
    EXPECT_EQ(buf[3], 0);
    EXPECT_EQ(buf[4], 'x');
    EXPECT_EQ(buf[7], 'x');
    EXPECT_EQ(buf[8], 0);

    for (size_t i = 0; i < sizeof(src); i++) { src[i] = (char) i; }
    ASSERT_EQ(memcpy(buf, src, sizeof(src)), (void *) buf);
    EXPECT_MEMEQ(buf, src, sizeof(src));

    memset(buf, 0x11, sizeof(buf));
    memcpy(buf, src, 0);
    memset(buf, 0x22, 0);
    EXPECT_EQ(buf[0], 0x11);
}

KERNEL_TEST(string_memmove_overlap) {
    char buf[16];

    memcpy(buf, "0123456789", 11);
    ASSERT_EQ(memmove(buf + 2, buf, 5), (void *) (buf + 2));
    EXPECT_MEMEQ(buf, "0101234789", 10);

    memcpy(buf, "0123456789", 11);
    ASSERT_EQ(memmove(buf, buf + 3, 5), (void *) buf);
    EXPECT_MEMEQ(buf, "3456756789", 10);

    memcpy(buf, "0123456789", 11);
    memmove(buf + 4, buf + 4, 3);
    EXPECT_MEMEQ(buf, "0123456789", 10);

    memmove(buf + 1, buf, 0);
    memmove(buf, buf + 1, 0);
    EXPECT_MEMEQ(buf, "0123456789", 10);
}

KERNEL_TEST(string_memcmp) {
    EXPECT_EQ(memcmp("abc", "abc", 3), 0);
    EXPECT_LT(memcmp("abc", "abd", 3), 0);
    EXPECT_GT(memcmp("abd", "abc", 3), 0);
    EXPECT_EQ(memcmp("abc", "abd", 2), 0);
    EXPECT_EQ(memcmp("x", "y", 0), 0);

    /* bytes compare as unsigned */
    EXPECT_GT(memcmp("\x80", "\x01", 1), 0);
    EXPECT_EQ(memcmp("a\0b", "a\0b", 3), 0);
    EXPECT_LT(memcmp("a\0a", "a\0b", 3), 0);
}

KERNEL_TEST(string_strlen) {
    EXPECT_EQ(strlen(""), 0);
    EXPECT_EQ(strlen("a"), 1);
    EXPECT_EQ(strlen("hello"), 5);
    EXPECT_EQ(strlen("ab\0cd"), 2);
    EXPECT_EQ(strlen(NULL), 0);

    EXPECT_EQ(strnlen("hello", 10), 5);
    EXPECT_EQ(strnlen("hello", 5), 5);
    EXPECT_EQ(strnlen("hello", 3), 3);
    EXPECT_EQ(strnlen("hello", 0), 0);
    EXPECT_EQ(strnlen("", 4), 0);
}

KERNEL_TEST(string_strcmp) {
    EXPECT_EQ(strcmp("", ""), 0);
    EXPECT_EQ(strcmp("abc", "abc"), 0);
    EXPECT_LT(strcmp("abc", "abd"), 0);
    EXPECT_GT(strcmp("abd", "abc"), 0);
    EXPECT_LT(strcmp("ab", "abc"), 0);
    EXPECT_GT(strcmp("abc", "ab"), 0);
    EXPECT_GT(strcmp("\x80", "a"), 0);

    EXPECT_EQ(strncmp("abcdef", "abcxyz", 3), 0);
    EXPECT_LT(strncmp("abcdef", "abcxyz", 4), 0);
    EXPECT_GT(strncmp("abcxyz", "abcdef", 4), 0);
    EXPECT_EQ(strncmp("abc", "abc", 100), 0);
    EXPECT_LT(strncmp("ab", "abc", 100), 0);
    EXPECT_EQ(strncmp("x", "y", 0), 0);
    EXPECT_GT(strncmp("\x80", "a", 1), 0);
}

KERNEL_TEST(string_strchr) {
    const char *s = "hello world";

    EXPECT_EQ(strchr(s, 'h'), s);
    EXPECT_EQ(strchr(s, 'o'), s + 4);
    EXPECT_EQ(strchr(s, 'd'), s + 10);
    EXPECT_NULL(strchr(s, 'z'));
    EXPECT_NULL(strchr("", 'a'));

    EXPECT_EQ(strrchr(s, 'o'), s + 7);
    EXPECT_EQ(strrchr(s, 'h'), s);
    EXPECT_EQ(strrchr(s, 'd'), s + 10);
    EXPECT_NULL(strrchr(s, 'z'));
    EXPECT_NULL(strrchr("", 'a'));

    EXPECT_EQ(strchr(s, '\0'), s + 11);
    EXPECT_EQ(strrchr(s, '\0'), s + 11);
}

KERNEL_TEST(string_strcpy) {
    char buf[16];

    memset(buf, 'x', sizeof(buf));
    ASSERT_EQ(strcpy(buf, "hello"), buf);
    EXPECT_STREQ(buf, "hello");
    EXPECT_EQ(buf[6], 'x');

    strcpy(buf, "");
    EXPECT_STREQ(buf, "");
    EXPECT_EQ(buf[1], 'e');
}

KERNEL_TEST(string_strncpy) {
    char buf[8];

    memset(buf, 'x', sizeof(buf));
    ASSERT_EQ(strncpy(buf, "abc", sizeof(buf)), buf);
    EXPECT_STREQ(buf, "abc");
    for (size_t i = 3; i < sizeof(buf); i++) { EXPECT_EQ(buf[i], 0); }

    memset(buf, 'x', sizeof(buf));
    strncpy(buf, "abcdefghij", 4);
    EXPECT_MEMEQ(buf, "abcdxxxx", 8);

    memset(buf, 'x', sizeof(buf));
    strncpy(buf, "abcd", 4);
    EXPECT_MEMEQ(buf, "abcdxxxx", 8);
}

KERNEL_TEST(string_strlcpy) {
    char buf[8];

    memset(buf, 'x', sizeof(buf));
    EXPECT_EQ(strlcpy(buf, "abc", 6), 3);
    EXPECT_STREQ(buf, "abc");
    EXPECT_EQ(buf[6], 'x');

    memset(buf, 'x', sizeof(buf));
    strlcpy(buf, "abcdefgh", 4);
    EXPECT_STREQ(buf, "abc");
    EXPECT_EQ(buf[4], 'x');

    memset(buf, 'x', sizeof(buf));
    strlcpy(buf, "abcd", 4);
    EXPECT_STREQ(buf, "abc");
    EXPECT_EQ(buf[4], 'x');

    EXPECT_EQ(strlcpy(buf, "abcdefgh", 4), 8);
}

static int fmt(char *buf, const char *f, ...) {
    va_list args;
    int     n;

    va_start(args, f);
    n = vsprintf(buf, f, args);
    va_end(args);

    return n;
}

KERNEL_TEST(string_sprintf_integers) {
    char buf[64];

    EXPECT_EQ(sprintf(buf, "%d", 42), 2);
    EXPECT_STREQ(buf, "42");
    sprintf(buf, "%d", -42);
    EXPECT_STREQ(buf, "-42");
    sprintf(buf, "%i", 0);
    EXPECT_STREQ(buf, "0");
    sprintf(buf, "%u", 4000000000u);
    EXPECT_STREQ(buf, "4000000000");
    sprintf(buf, "%x", 0xbeef);
    EXPECT_STREQ(buf, "beef");
    sprintf(buf, "%X", 0xbeef);
    EXPECT_STREQ(buf, "BEEF");
    sprintf(buf, "%o", 8);
    EXPECT_STREQ(buf, "10");

    sprintf(buf, "%ld", -1234567890123l);
    EXPECT_STREQ(buf, "-1234567890123");
    sprintf(buf, "%lu", ~0ul);
    EXPECT_STREQ(buf, "18446744073709551615");
    sprintf(buf, "%lx", ~0ul);
    EXPECT_STREQ(buf, "ffffffffffffffff");
    sprintf(buf, "%ld", (long) (~0ul >> 1));
    EXPECT_STREQ(buf, "9223372036854775807");

    sprintf(buf, "%x", -1);
    EXPECT_STREQ(buf, "ffffffff");
}

KERNEL_TEST(string_sprintf_flags) {
    char buf[64];

    sprintf(buf, "%5d", 42);
    EXPECT_STREQ(buf, "   42");
    sprintf(buf, "%5d", -42);
    EXPECT_STREQ(buf, "  -42");
    sprintf(buf, "%-5d|", 42);
    EXPECT_STREQ(buf, "42   |");
    sprintf(buf, "%05d", 42);
    EXPECT_STREQ(buf, "00042");
    sprintf(buf, "%05d", -42);
    EXPECT_STREQ(buf, "-0042");
    sprintf(buf, "%+d %+d", 5, -5);
    EXPECT_STREQ(buf, "+5 -5");
    sprintf(buf, "% d", 5);
    EXPECT_STREQ(buf, " 5");
    sprintf(buf, "%.3d", 5);
    EXPECT_STREQ(buf, "005");
    sprintf(buf, "%#x", 255);
    EXPECT_STREQ(buf, "0xff");
    sprintf(buf, "%#X", 255);
    EXPECT_STREQ(buf, "0XFF");
    sprintf(buf, "%#o", 8);
    EXPECT_STREQ(buf, "010");
    sprintf(buf, "%#010lx", 0x1234ul);
    EXPECT_STREQ(buf, "0x00001234");
    sprintf(buf, "%*d", 4, 7);
    EXPECT_STREQ(buf, "   7");
    sprintf(buf, "%*d|", -4, 7);
    EXPECT_STREQ(buf, "7   |");
    sprintf(buf, "%p", (void *) 0x1234);
    EXPECT_STREQ(buf, "0000000000001234");
}

KERNEL_TEST(string_sprintf_strings) {
    char buf[64];
    int  n = -1;

    sprintf(buf, "%s", "hi");
    EXPECT_STREQ(buf, "hi");
    sprintf(buf, "%5s", "hi");
    EXPECT_STREQ(buf, "   hi");
    sprintf(buf, "%-5s|", "hi");
    EXPECT_STREQ(buf, "hi   |");
    sprintf(buf, "%.2s", "hello");
    EXPECT_STREQ(buf, "he");
    sprintf(buf, "%s", (char *) NULL);
    EXPECT_STREQ(buf, "<NULL>");
    sprintf(buf, "%c%c", 'o', 'k');
    EXPECT_STREQ(buf, "ok");
    sprintf(buf, "%3c", 'a');
    EXPECT_STREQ(buf, "  a");
    sprintf(buf, "100%%");
    EXPECT_STREQ(buf, "100%");
    sprintf(buf, "abc%n", &n);
    EXPECT_EQ(n, 3);

    EXPECT_EQ(fmt(buf, "%s=%d", "x", 10), 4);
    EXPECT_STREQ(buf, "x=10");
    EXPECT_EQ(sprintf(buf, ""), 0);
    EXPECT_STREQ(buf, "");
}

KERNEL_TEST(string_strtou) {
    char *end;

    EXPECT_EQ(strtou("123", &end, 10), 123);
    EXPECT_EQ(*end, '\0');
    EXPECT_EQ(strtou("12abc", &end, 10), 12);
    EXPECT_STREQ(end, "abc");
    EXPECT_EQ(strtou("ff", NULL, 16), 255);
    EXPECT_EQ(strtou("FF", NULL, 16), 255);
    EXPECT_EQ(strtou("777", NULL, 8), 0777);
    EXPECT_EQ(strtou("789", &end, 8), 7);
    EXPECT_STREQ(end, "89");
    EXPECT_EQ(strtou("1010", NULL, 2), 10);
    EXPECT_EQ(strtou("102", &end, 2), 2);
    EXPECT_STREQ(end, "2");

    EXPECT_EQ(strtou("0x1F", &end, 0), 0x1F);
    EXPECT_EQ(*end, '\0');
    EXPECT_EQ(strtou("017", NULL, 0), 017);
    EXPECT_EQ(strtou("42", NULL, 0), 42);
    EXPECT_EQ(strtou("0", &end, 0), 0);
    EXPECT_EQ(*end, '\0');

    const char *junk = "xyz";
    EXPECT_EQ(strtou(junk, &end, 10), 0);
    EXPECT_EQ((const char *) end, junk);
}

KERNEL_TEST(ctype_classes) {
    for (int c = 0; c < 256; c++) {
        bool digit = c >= '0' && c <= '9';
        bool lower = c >= 'a' && c <= 'z';
        bool upper = c >= 'A' && c <= 'Z';
        bool hex   = digit || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');

        EXPECT_EQ(!!isdigit(c), digit);
        EXPECT_EQ(!!islower(c), lower);
        EXPECT_EQ(!!isupper(c), upper);
        EXPECT_EQ(!!isxdigit(c), hex);
        if (TEST_HAS_FAILED()) {
            ADD_FAILURE("first misclassified character: %d", c);
            return;
        }
    }
}

KERNEL_TEST(ctype_case_conversion) {
    for (int c = 0; c < 256; c++) {
        int lower = (c >= 'A' && c <= 'Z') ? c + ('a' - 'A') : c;
        int upper = (c >= 'a' && c <= 'z') ? c - ('a' - 'A') : c;

        EXPECT_EQ(tolower(c), lower);
        EXPECT_EQ(toupper(c), upper);
        if (TEST_HAS_FAILED()) {
            ADD_FAILURE("first mis-converted character: %d", c);
            return;
        }
    }
}
