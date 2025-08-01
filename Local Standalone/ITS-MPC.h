// ITS-MPC.h

#ifndef ITS_MPC_H
#define ITS_MPC_H

// Libraries
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// Constants
#define NP 3
#define MOD 10289 // 1009
#define GAMMA1 3
#define GAMMA2 (MOD - 3)
#define GAMMA3 1
extern int32_t gammaArray[NP];

// Function Prototypes

int32_t SPLIT(int32_t j, int32_t r, int32_t p);
int32_t RECONSTRUCT(int32_t shares[]);
void RENORMALIZE(int32_t shares[]);

void MultTest(int a, int b);
void Compare2Test(int u, int v);

// N Share Functions **Not Implemented**
// int RECONSTRUCT(int *sharesArr, int n);
// void RENORMALIZE(int *sharesArr, int n);

#endif // ITS_MPC_H