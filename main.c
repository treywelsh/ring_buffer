#include <stdio.h>
#include "ringb.h"

int test_case(void);

int
test_case(void) {
    ringb_t b = {0};
    size_t i;
    int a[] = { 5, 6, 3, 4, 2, 6, 7};
    size_t a_len = 7;
    unsigned int elt;
    size_t count;

    /* display array for visual test check */
    printf("array: ");
    for (i = 0 ; i < a_len ; i++) {
        printf("%d ", a[i]);
    }
    printf("\n\n");

    /* add integers to the ring buffer */
    ringb_init(&b, 3);
    printf("size %u\n", b.bufmask + 1);
    for (i = 0 ; i < 30 ; i++) {
        if (ringb_is_full(&b)) {
            printf("total:%zu elements added\n\n", i);
            break;
        }
        ringb_add_unsafe(&b, a[i]);
        printf("%d added to the ring \n", a[i]);
    }

    count = 0;
    while (!ringb_get(&b, &elt)) {
        printf("retrieved from the buffer : %d\n", elt);
        count++;
    }
    printf("total:%zu elements retrieved\n", count);

    ringb_clean(&b);

    return 0;
}

int
main (int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    test_case();

    return 0;
}
