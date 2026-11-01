#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>


int main(){


	struct timeval start, end;
	gettimeofday(&start, NULL);

	sleep(1);

	gettimeofday(&end, NULL);
	double elapsed =
	(end.tv_sec - start.tv_sec) +
	(end.tv_usec - start.tv_usec) / 1000000.0;
	
	printf("Time = %.6f seconds\n", elapsed);

	return 0;
}
