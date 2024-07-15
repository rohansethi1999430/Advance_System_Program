#include <stdio.h>
#include <unistd.h>

int main() {
    while (1) {
        printf("Running in an infinite loop...\n");
        fflush(stdout);
        sleep(1); // Sleep for 1 second to avoid flooding the output
    }
    return 0;
}
