/* 
 Based on Advanced Programming in the Unix Envitonment, 3ed
 by R. Stevens and S. Rago
 */

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>

#define NTHR   8    				/* number of threads */
#define NUMNUM 16000000L		/* number of numbers to sort */
#define TNUM   (NUMNUM/NTHR)	/* number to sort per thread */

long nums[NUMNUM];
long snums[NUMNUM];

int compareInts(const void *arg1, const void *arg2);
void * threadFunction(void *arg);
void merge();
double calcTime(struct timeval start);

int main()
{
	unsigned long	i;
	struct timeval	start;
	double			elapsed;
	int				err;
	pthread_t		tid;

	/*
	 * Create the initial set of numbers to sort.
	 */
	srandom(1);
    for (i = 0; i < NUMNUM; i++){
		nums[i] = random();
    }
    
    /*
     * Get timing data
     */
	gettimeofday(&start, NULL);
    
    /*
     * Create NTHR threads to sort the numbers.
     */

    for (i = 0; i < NTHR; i++) {
        long startOffset = i * TNUM;
        err = pthread_create(&tid, NULL, threadFunction, (void *)startOffset);
        if (err != 0){
            printf("Cannot create thread, error: %d", err);
            exit(-1);
        }
    }
    
    /*
     * Wait for workers to finish and merge the sorter arrays
     */
    
    for (i = 0; i < NTHR; i++){
        pthread_join(tid, NULL);
    }
        
    merge();
    
    /*
     * Get and display elapsed wall time time
     */
    elapsed = calcTime(start);
	printf("sort took %.4f seconds\n", elapsed);

	exit(0);
}

/*
 * Compare two long integers (helper function for heapsort)
 */
int compareInts(const void *arg1, const void *arg2)
{
    long l1 = *(long *)arg1;
    long l2 = *(long *)arg2;
    
    if (l1 == l2)
        return 0;
    else if (l1 < l2)
        return -1;
    else
        return 1;
}

/*
 * Worker thread to sort a portion of the set of numbers.
 */
void * threadFunction(void *arg)
{
    long	startIndex = (long)arg;
    
    qsort(&nums[startIndex], TNUM, sizeof(long), compareInts);
    
    return((void *)0);
}

/*
 * Merge the results of the individual sorted ranges.
 */
void merge()
{
    long	index[NTHR];
    long	i, minIndex, sindex, num;
    
    for (i = 0; i < NTHR; i++)
        index[i] = i * TNUM;
    
    for (sindex = 0; sindex < NUMNUM; sindex++) {
        num = LONG_MAX;
        for (i = 0; i < NTHR; i++) {
            if ((index[i] < (i+1)*TNUM) && (nums[index[i]] < num)) {
                num = nums[index[i]];
                minIndex = i;
            }
        }
        snums[sindex] = nums[index[minIndex]];
        index[minIndex]++;
    }
}

double calcTime(struct timeval start){
    
    long long		startusec, endusec;
    struct timeval	end;
    
    gettimeofday(&end, NULL);
    startusec = start.tv_sec * 1000000 + start.tv_usec;
    endusec = end.tv_sec * 1000000 + end.tv_usec;
    return (double)(endusec - startusec) / 1000000.0;
}
