#include<stdio.h>
#include<assert.h>
#include<sys/time.h>

uint32_t NDL_GetTicks();

int main()
{
   //struct timeval now = (struct timeval*)malloc(sizeof(struct timeval));
   //struct timeval now;
   time_t time = 500;
   while(1)
   {
	   // gettimeofday(&now, NULL);
	   uint32_t now = 0;
	   while (now < time)
	   		now = NDL_GetTicks();
	   
	   time += 500;
	   printf("Pass 0.5s, now = %dms\n", now);
   }
}
