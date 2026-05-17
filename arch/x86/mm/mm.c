#include <mm/mm_types.h>
#include <nyx/string.h>

void __mm_copy_higher_half(struct mm_struct *dest, struct mm_struct *src) {
    memcpy(&dest->pgd[256], &src->pgd[256], 256 * sizeof(pgd_t));
}
