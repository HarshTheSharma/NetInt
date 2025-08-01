#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define JOIN_MSG    "JOIN\n"
#define NP 3
#define MOD 10289 // 1009
#define GAMMA1 3
#define GAMMA2 (MOD - 3)
#define GAMMA3 1
#define l 14 // calculated as l = (int32_t)(log2(MOD - 1)) + 1; but needs to be constant

/* ------------ protocol codes ------------ */
enum { OP_ADD = 0x01,
       OP_MUL = 0x02,
       OP_CMP = 0x03,
       OP_EQL = 0x04,
       OP_REN = 0x80,
       OP_RES = 0x81 };

/* fixed-size packets – sent in network byte-order */
typedef struct __attribute__((packed)) {
    uint8_t op;
    uint32_t a;
    uint32_t b;
    int32_t u_shares[l];
    int32_t v_shares[l];
} task_t;

typedef struct __attribute__((packed)) {
    uint8_t  op;
    uint32_t value;
} response_t;

int lookup_and_connect(const char *host, const char *service);
int bind_and_listen   (const char *service);

#endif /* COMMON_H */
