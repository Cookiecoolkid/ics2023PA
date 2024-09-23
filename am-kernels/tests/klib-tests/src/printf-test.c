#include<klib.h>

int main()
{
	int d = 221240093;
	char * s = "hello";
	printf("%s, %d!\n", s, d);
	for(int i = 0; i < 24; i++)
		printf("%s, %02d:%02d:%02d!\n", s, i, i + 1, i + 2);
	return 0;
}
