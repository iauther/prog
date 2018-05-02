
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include "sf.h"


//#define NK_IMPLEMENTATION
//#include "nuklear.h"



#define UBOOT_FILE		"uboot.bin"
#define UBOOT_ADDR		0

#define SYSTEM_FILE		"widroa.bin"
#define SYSTEM_ADDR		0x5000

int main(int argc, char *argv[])
{
	bool  r;
	int   rc = 0;

	r = sf_init();
	if (r) {

		//sf_erase();
		r = sf_prog(UBOOT_ADDR, UBOOT_FILE);
		if (!r) {
			printf("___ sf_prog failed\n");
		}
	}
	else {
		printf("___ sf_init failed\n");
	}

	r = sf_free();

	system("pause");
	
	return rc;
}
