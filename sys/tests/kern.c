#include <sys/errno.h>
#include <sys/irq.h>
#include <sys/kernel.h>
#include <sys/string.h>
#include <sys/testing.h>
#include <sys/uio.h>

#include <asi/irq_vectors.h>

KERNEL_TEST(uio_sysspace_read) {
    char         a[4], b[3], c[8];
    struct iovec iov[3] = {{a, sizeof(a)}, {b, sizeof(b)}, {c, sizeof(c)}};
    struct uio   uio    = {
            .uio_iov    = iov,
            .uio_iovcnt = 3,
            .uio_offset = 100,
            .uio_resid  = sizeof(a) + sizeof(b) + sizeof(c),
            .uio_rw     = UIO_READ,
            .uio_segflg = UIO_SYSSPACE,
    };

    memset(a, 0, sizeof(a));
    memset(b, 0, sizeof(b));
    memset(c, '-', sizeof(c));

    EXPECT_OK(uiomove("0123", 4, &uio));
    EXPECT_OK(uiomove("45678", 5, &uio));

    EXPECT_MEMEQ(a, "0123", 4);
    EXPECT_MEMEQ(b, "456", 3);
    EXPECT_MEMEQ(c, "78------", 8);
    EXPECT_EQ(uio.uio_resid, 6);
    EXPECT_EQ(uio.uio_offset, 109);

    /* moves stop when the iovecs are full */
    EXPECT_OK(uiomove("9abcdefgh", 9, &uio));
    EXPECT_MEMEQ(c, "789abcde", 8);
    EXPECT_EQ(uio.uio_resid, 0);
    EXPECT_EQ(uio.uio_offset, 115);

    EXPECT_OK(uiomove("zzzz", 4, &uio));
    EXPECT_MEMEQ(c, "789abcde", 8);
    EXPECT_EQ(uio.uio_offset, 115);
}

KERNEL_TEST(uio_sysspace_write) {
    char         out[16];
    struct iovec iov[3] = {{"ab", 2}, {"", 0}, {"cdefg", 5}};
    struct uio   uio    = {
            .uio_iov    = iov,
            .uio_iovcnt = 3,
            .uio_offset = 0,
            .uio_resid  = 7,
            .uio_rw     = UIO_WRITE,
            .uio_segflg = UIO_SYSSPACE,
    };

    memset(out, 0, sizeof(out));

    EXPECT_OK(uiomove(out, 3, &uio));
    EXPECT_STREQ(out, "abc");
    EXPECT_EQ(uio.uio_resid, 4);
    EXPECT_EQ(uio.uio_offset, 3);
    EXPECT_EQ(uio.uio_iov, &iov[2]);
    EXPECT_EQ(iov[2].iov_len, 4);

    EXPECT_OK(uiomove(out + 3, 10, &uio));
    EXPECT_STREQ(out, "abcdefg");
    EXPECT_EQ(uio.uio_resid, 0);
    EXPECT_EQ(uio.uio_offset, 7);
}

static int irq_hits;

static void test_irq_handler(void *ctx) {
    irq_hits += *(int *) ctx;
}

static void other_irq_handler(void *ctx) {
    (void) ctx;
}

KERNEL_TEST(irq_install_remove) {
    static int   ctx = 1;
    unsigned int irq = NR_IRQS - 1;

    EXPECT_EQ(irq_install_handler(NR_IRQS, test_irq_handler, "test", &ctx), -EINVAL);
    EXPECT_EQ(irq_remove_handler(NR_IRQS, test_irq_handler), -EINVAL);
    EXPECT_EQ(irq_remove_handler(irq, test_irq_handler), -ENOENT);

    ASSERT_OK(irq_install_handler(irq, test_irq_handler, "test", &ctx));
    EXPECT_EQ(irq_remove_handler(irq, other_irq_handler), -ENOENT);
    EXPECT_OK(irq_remove_handler(irq, test_irq_handler));
    EXPECT_EQ(irq_remove_handler(irq, test_irq_handler), -ENOENT);
}

KERNEL_TEST(irq_remove_among_many) {
    static int   ctx = 1;
    unsigned int irq = NR_IRQS - 2;

    ASSERT_OK(irq_install_handler(irq, test_irq_handler, "first", &ctx));
    ASSERT_OK(irq_install_handler(irq, other_irq_handler, "second", &ctx));

    EXPECT_OK(irq_remove_handler(irq, test_irq_handler));
    EXPECT_OK(irq_remove_handler(irq, other_irq_handler));
    EXPECT_EQ(irq_remove_handler(irq, other_irq_handler), -ENOENT);

    ASSERT_OK(irq_install_handler(irq, test_irq_handler, "first", &ctx));
    ASSERT_OK(irq_install_handler(irq, other_irq_handler, "second", &ctx));

    EXPECT_OK(irq_remove_handler(irq, other_irq_handler));
    EXPECT_OK(irq_remove_handler(irq, test_irq_handler));
    EXPECT_EQ(irq_remove_handler(irq, test_irq_handler), -ENOENT);
}
