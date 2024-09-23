#include <klib.h>

#define N 25
char str[N];
void reset() 
{
	int i;
	for (i = 0; i < N; i ++) 
		str[i] = 'a' + i;
}

void check_seq(int l, int r, int val)
{
	int i;
	for (i = l; i < r; i ++) 
		assert(str[i] == val + i - l);
}

void check_eq(int l, int r, char* test)
{
	int i;
	int index = 0;
	for (i = l; i < r; i ++)
		assert(str[i] == test[index++]);
}
void test_strcpy() 
{
	int l;
	char test[] = "hello";
	int len = strlen(test);
	assert(len == 5);
    for (l = 0; l < N - len; l ++) 
	{
		reset();
		strcpy(str + l, test);
       	check_seq(0, l, 'a');
       	check_eq(l, l + len, test);
       	check_seq(l + len, N, l + len + 'a');
    }
}

int main()
{
	test_strcpy();
}
