#include <stdio.h>
#include <ctype.h>

void print_arguments(int argc, char *argv[]) {
    int i = 0;
    for (i = 0; i < argc; i++) {
        int j = 0;
        while (argv[i][j] != '\0') {
            char ch = argv[i][j];
            if (isalpha(ch) || isblank(ch)) {
                printf("'%c' == %d ", ch, ch);
            }
            j++;
        }
        printf("\n");
    }
}

int main(int argc, char *argv[]) {
    print_arguments(argc, argv);
    return 0;
}
