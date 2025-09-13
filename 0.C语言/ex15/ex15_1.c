#include <stdio.h>

int main(int argc, char *argv[]) {
    // create two arrays we care about
    int ages[] = {23, 43, 12, 89, 2};
    char *names[] = {
        "Alan", "Frank",
        "Mary", "John", "Lisa"
    };
    // safely get the size of ages
    int count = sizeof(ages) / sizeof(int);
    int *cur_age = ages;
    char **cur_name = names;

    // first way using pointers
    for (int i = 0; i < count; i++) {
        printf("%s has %d years alive.\n",
               *(cur_name + i), *(cur_age + i));
    }
    printf("---\n");

    return 0;
}
