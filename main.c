#include <stdio.h>
#include <string.h>
#include "assembler.h"

int main(int argc, char* argv[]) {
	if (argc > 1) {
		if (argc > 2 && (strcmp(argv[1], "-w") == 0 || strcmp(argv[1], "--watch") == 0)) {
			// watching
			printf("watching %s...\n", argv[2]);
		} else {
			// assembling
			printf("assembling '%s'\n", argv[1]);
			if (cca_assemble(argv[1])) {
				puts("done!");
			} else {
				puts("failed to assemble due to errors");
			}
		}
	}

	return 1;
}