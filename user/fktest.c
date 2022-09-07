#include "lib.h"


void umain()
{
	int a = 0;
	int id = 0;

	if ((id = fork()) == 0) {
//		writef("hhhhh 1 fork id==%d\n",id);
		if ((id = fork()) == 0) {
//		writef("hhhhh 2 fork\n");
			a += 3;

			for (;;) {
				writef("\t\tthis is child2 :a:%d\n", a);
			}
		}
//writef("a+=2\n");
		a += 2;

		for (;;) {
			writef("\tthis is child :a:%d\n", a);
		}
	}

	a++;

	for (;;) {
		writef("this is father: a:%d\n", a);
	}
}
