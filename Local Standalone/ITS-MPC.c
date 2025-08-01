// ITS-MPC.c

#include "ITS-MPC.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Coefficients for reconstruction
int32_t gammaArray[NP] = {GAMMA1, GAMMA2, GAMMA3};

/*********************************************************************************
 * @brief Used to split secret shares for agents.
 * @param int32_t j: party index (0-based)
 * @param int32_t r: random coefficient
 * @param int32_t p: secret value
 * @return int32_t: share for party j
 *********************************************************************************/
int32_t SPLIT(int32_t j, int32_t r, int32_t p) {
	return (((j + 1) * r + p) + MOD) % MOD;
}

/*********************************************************************************
 * @brief Reconstruct secret from shares using gamma coefficients.
 * @param int32_t shares[]: array of shares
 * @return int32_t: reconstructed secret
 *********************************************************************************/
int32_t RECONSTRUCT(int32_t shares[]) {
	int32_t result = 0;
	for (int32_t j = 0; j < NP; j++) {
		result = (result + gammaArray[j] * shares[j]) % MOD;
	}
	if (result < 0) result += MOD;
	return result;
}

/*********************************************************************************
 * @brief Renormalize shares to reduce polynomial degree.
 * @param int32_t shares[]: array of shares to renormalize (modified in-place)
 * @note Based on Protocol 2.
 *********************************************************************************/
void RENORMALIZE(int32_t shares[]) {
	int32_t r_U = rand() % MOD;
	int32_t coeff_r2 = rand() % MOD;

	// Step 1: Generate random shares [r_U]
	int32_t share_r[NP];
	for (int32_t j = 0; j < NP; j++) {
		share_r[j] = (r_U + coeff_r2 * (j + 1)) % MOD;
	}

	// Step 2: Compute d_j = s_j + [r_U]_j
	int32_t d[NP];
	for (int32_t j = 0; j < NP; j++) {
		d[j] = (shares[j] + share_r[j]) % MOD;
	}

	// Step 3: Reshare each d_j to create [d_j]^k for all parties
	int32_t reshare_d[NP][NP]; // reshare_d[j][k] = share of d[j] for party k
	for (int32_t j = 0; j < NP; j++) {
		int32_t coeff = rand() % MOD;
		for (int32_t k = 0; k < NP; k++) {
			reshare_d[j][k] = (d[j] + coeff * (k + 1)) % MOD;
		}
	}

	// Step 4: Compute new shares using gamma coefficients
	for (int32_t k = 0; k < NP; k++) {
		int32_t sum = 0;
		for (int32_t j = 0; j < NP; j++) {
			sum = (sum + gammaArray[j] * reshare_d[j][k]) % MOD;
		}
		shares[k] = (sum - share_r[k]) % MOD;
		if (shares[k] < 0) shares[k] += MOD;
	}
}

void MultTest(int32_t a, int32_t b) {
	// Split a int32_to shares
	int32_t r1 = rand() % MOD;
	int32_t shares_a[NP];
	for (int32_t j = 0; j < NP; j++) {
		shares_a[j] = SPLIT(j, r1, a);
	}

	// Split b int32_to shares
	int32_t r2 = rand() % MOD;
	int32_t shares_b[NP];
	for (int32_t j = 0; j < NP; j++) {
		shares_b[j] = SPLIT(j, r2, b);
	}

	// Agent actions: Local multiplication
	int32_t prod_shares[NP];
	for (int32_t j = 0; j < NP; j++) {
		prod_shares[j] = (shares_a[j] * shares_b[j]) % MOD;
	}

	RENORMALIZE(prod_shares); // Renormalize product shares

	// Reconstruction
	int32_t reconstructed = RECONSTRUCT(prod_shares);
	int32_t expected = (a * b) % MOD;

	printf("a = %d, b = %d\n", a, b);
	printf("Reconstructed product: %d\n", reconstructed);
	printf("Expected product: %d\n", expected);
}

void ADDTest(int32_t a, int32_t b) {
	// Split a int32_to shares
	int32_t r1 = rand() % MOD;
	int32_t shares_a[NP];
	for (int32_t j = 0; j < NP; j++) {
		shares_a[j] = SPLIT(j, r1, a);
	}

	// Split b int32_to shares
	int32_t r2 = rand() % MOD;
	int32_t shares_b[NP];
	for (int32_t j = 0; j < NP; j++) {
		shares_b[j] = SPLIT(j, r2, b);
	}

	// Agent actions: Local multiplication
	int32_t prod_shares[NP];
	for (int32_t j = 0; j < NP; j++) {
		prod_shares[j] = (shares_a[j] + shares_b[j]) % MOD;
	}

	// Reconstruction
	int32_t reconstructed = RECONSTRUCT(prod_shares);

	printf("a = %d, b = %d\n", a, b);
	printf("Reconstructed sum: %d\n", reconstructed);
}

void Compare2Test(int32_t u, int32_t v) {
	// Calculate MAX_BITS based on MOD using log2
	int32_t l = (int32_t)(log2(MOD - 1)) + 1;
	printf("Number of bits required for MOD %d: %d\n", MOD, l);
	// Static arrays sized for worst case (MOD)
	int32_t bits_u[l];
	int32_t bits_v[l];
	int32_t u_shares[l][NP];
	int32_t v_shares[l][NP];

	// Initialize bit arrays to 0
	for (int32_t i = 0; i < l; i++) {
		bits_u[i] = 0;
		bits_v[i] = 0;
	}

	// Convert u and v to bit arrays
	for (int32_t i = 0; i < l; i++) {
		bits_u[i] = (u >> (l - 1 - i)) & 1;
		bits_v[i] = (v >> (l - 1 - i)) & 1;
	}

	// Share all bits of u and v
	for (int32_t i = 0; i < l; i++) {
		int32_t ru = rand() % MOD;
		int32_t rv = rand() % MOD;
		for (int32_t j = 0; j < NP; j++) {
			u_shares[i][j] = SPLIT(j, ru, bits_u[i]);
			v_shares[i][j] = SPLIT(j, rv, bits_v[i]);
		}
	}

	// Initialize b_shares
	int32_t b_shares[NP];
	int32_t rb0 = rand() % MOD;
	for (int32_t j = 0; j < NP; j++) {
		b_shares[j] = SPLIT(j, rb0, 1);
	}

	// Initialize c_shares: [c] = [u0 - v0 + 1]
	int32_t c_shares[NP];
	for (int32_t j = 0; j < NP; j++) {
		c_shares[j] = (u_shares[0][j] - v_shares[0][j] + 1 + MOD) % MOD;
	}

	// Agent actions: Process remaining bits
	for (int32_t i = 1; i < l; i++) {
		// XOR(u_{i-1}, v_{i-1}) = u + v - 2uv
		int32_t uv_prod[NP];
		for (int32_t j = 0; j < NP; j++) {
			uv_prod[j] = (u_shares[i - 1][j] * v_shares[i - 1][j]) % MOD;
		}
		RENORMALIZE(uv_prod);

		int32_t xor_shares[NP];
		for (int32_t j = 0; j < NP; j++) {
			xor_shares[j] = (u_shares[i - 1][j] + v_shares[i - 1][j] - 2 * uv_prod[j] + MOD) % MOD;
		}
		RENORMALIZE(xor_shares);

		// tmp = (1 - XOR)
		int32_t tmp_shares[NP];
		for (int32_t j = 0; j < NP; j++) {
			tmp_shares[j] = (1 - xor_shares[j] + MOD) % MOD;
		}
		RENORMALIZE(tmp_shares);

		// b = b * tmp
		for (int32_t j = 0; j < NP; j++) {
			b_shares[j] = (b_shares[j] * tmp_shares[j]) % MOD;
		}
		RENORMALIZE(b_shares);

		// t = (u_i - v_i + 2 - b)
		int32_t t_shares[NP];
		for (int32_t j = 0; j < NP; j++) {
			t_shares[j] = (u_shares[i][j] - v_shares[i][j] + 2 - b_shares[j] + MOD) % MOD;
		}
		RENORMALIZE(t_shares);

		// c = c * t
		for (int32_t j = 0; j < NP; j++) {
			c_shares[j] = (c_shares[j] * t_shares[j]) % MOD;
		}
		RENORMALIZE(c_shares);
	}

	// Reconstruction and result check
	int32_t result = RECONSTRUCT(c_shares) ? 1 : 0;
	int32_t expected = (u >= v);
	printf("u = %d, v = %d\nReconstructed Result = %d\nExpected Result = %d\n", u, v, result, expected);
}

/*********************************************************************************
 * @brief MPC parallel comparison protocol: returns c=1 if u>=v, c=0 otherwise,
 *         and e=1 if u=v, e=0 otherwise.
 * @param u: First value to compare
 * @param v: Second value to compare
 * @param c: Output indicating u>=v (1) or u<v (0)
 * @param e: Output indicating equality (1) or not (0)
 *********************************************************************************/
void CompareParallelTest(int32_t u, int32_t v, int32_t *c, int32_t *e) {
	// Determine bit length based on MOD
	int32_t l = (int32_t)(log2(MOD - 1)) + 1;

	// Server Actions:
	// Convert u and v to bit arrays (MSB first)
	int32_t bits_u[l], bits_v[l];
	int32_t u_shares[l][NP], v_shares[l][NP], eq_shares[l][NP], gt_shares[l][NP], lt_shares[l][NP], prefixEq_shares[l][NP], flag_shares[l][NP];
	for (int32_t i = 0; i < l; i++) {
		bits_u[i] = (u >> (l - 1 - i)) & 1;
		bits_v[i] = (v >> (l - 1 - i)) & 1;
	}

	memset(eq_shares, 0, sizeof(eq_shares));
	memset(gt_shares, 0, sizeof(gt_shares));
	memset(lt_shares, 0, sizeof(lt_shares));
	memset(prefixEq_shares, 0, sizeof(prefixEq_shares));
	memset(flag_shares, 0, sizeof(flag_shares));

	// Share bits of u and v
	for (int32_t i = 0; i < l; i++) {
		int32_t ru = rand() % MOD;
		int32_t rv = rand() % MOD;
		for (int32_t j = 0; j < NP; j++) {
			u_shares[i][j] = SPLIT(j, ru, bits_u[i]);
			v_shares[i][j] = SPLIT(j, rv, bits_v[i]);
		}
	}
	int32_t tmp_shares[NP], tmp2_shares[NP];

	// Agent Actions: (Renorm needs server intervention as always)
	// Compute [eq_j], [gt_j], [lt_j] for all bits
	for (int32_t j = 0; j < l; j++) {
		// [eq_j] = 1 - ([u_j] XOR [v_j])
		// [xor] = [u_j] + [v_j] - 2*[u_j*v_j]
		for (int32_t k = 0; k < NP; k++) {
			tmp_shares[k] = (u_shares[j][k] * v_shares[j][k]) % MOD;
		}
		RENORMALIZE(tmp_shares); // [u_j*v_j]

		for (int32_t k = 0; k < NP; k++) {
			tmp2_shares[k] = (u_shares[j][k] + v_shares[j][k] - 2 * tmp_shares[k] + MOD) % MOD;
		}
		for (int32_t k = 0; k < NP; k++) {
			eq_shares[j][k] = (1 - tmp2_shares[k] + MOD) % MOD;
		}

		// [gt_j] = [u_j] * (1 - [v_j])
		for (int32_t k = 0; k < NP; k++) {
			tmp_shares[k] = (1 - v_shares[j][k] + MOD) % MOD; // [not_v_j]
		}
		for (int32_t k = 0; k < NP; k++) {
			gt_shares[j][k] = (u_shares[j][k] * tmp_shares[k]) % MOD;
		}
		RENORMALIZE(gt_shares[j]); // [gt_j]

		// [lt_j] = (1 - [u_j]) * [v_j]
		for (int32_t k = 0; k < NP; k++) {
			tmp_shares[k] = (1 - u_shares[j][k] + MOD) % MOD; // [not_u_j]
		}
		for (int32_t k = 0; k < NP; k++) {
			lt_shares[j][k] = (tmp_shares[k] * v_shares[j][k]) % MOD;
		}
		RENORMALIZE(lt_shares[j]); // [lt_j]
	}

	// Compute prefix equalities [prefixEq_j]
	// Initialize [prefixEq_0] = 1
	int32_t r0 = rand() % MOD;
	for (int32_t k = 0; k < NP; k++) {
		prefixEq_shares[0][k] = SPLIT(k, r0, 1);
	}

	for (int32_t j = 1; j < l; j++) {
		// [prefixEq_j] = [prefixEq_{j-1}] * [eq_{j-1}]
		for (int32_t k = 0; k < NP; k++) {
			tmp_shares[k] = (prefixEq_shares[j - 1][k] * eq_shares[j - 1][k]) % MOD;
		}
		RENORMALIZE(tmp_shares);
		for (int32_t k = 0; k < NP; k++) {
			prefixEq_shares[j][k] = tmp_shares[k];
		}
	}

	// [flag_j] = [prefixEq_j] * ([gt_j] - [lt_j])
	for (int32_t j = 0; j < l; j++) {
		// [diff_j] = [gt_j] - [lt_j]
		for (int32_t k = 0; k < NP; k++) {
			tmp_shares[k] = (gt_shares[j][k] - lt_shares[j][k] + MOD) % MOD;
		}
		// [flag_j] = [prefixEq_j] * [diff_j]
		for (int32_t k = 0; k < NP; k++) {
			tmp2_shares[k] = (prefixEq_shares[j][k] * tmp_shares[k]) % MOD;
		}
		RENORMALIZE(tmp2_shares);
		for (int32_t k = 0; k < NP; k++) {
			flag_shares[j][k] = tmp2_shares[k];
		}
	}

	// [cmp] = sum_j [flag_j]
	int32_t cmp_shares[NP] = {0};
	for (int32_t j = 0; j < l; j++) {
		for (int32_t k = 0; k < NP; k++) {
			cmp_shares[k] = (cmp_shares[k] + flag_shares[j][k]) % MOD;
		}
	}

	// Reconstruct cmp and calculate c and e
	int32_t cmp = RECONSTRUCT(cmp_shares);
	if (cmp >= 0 && cmp <= (MOD / 2))
		*c = 1;
	else
		*c = 0;

	if (cmp == 0)
		*e = 1;
	else
		*e = 0;
}

// Example usage:
int main(int argc, char *argv[]) {
	if (argc < 3) {
		printf("Usage: %s <a> <b>\n", argv[0]);
		return 1;
	}
	int32_t a = atoi(argv[1]);
	int32_t b = atoi(argv[2]);
	srand(time(NULL));
	MultTest(a, b);
	ADDTest(a, b);
	int32_t c, e;
	CompareParallelTest(a, b, &c, &e);
	printf("MPC Parallel Comparison: u=%d, v=%d\n", a, b);
	printf("\tc = %d (u ", c);
	if (c) {
		printf(">= v)\n");
	} else {
		printf("< v)\n");
	}

	printf("\te = %d (u ", e);
	if (e) {
		printf("== v)\n");
	} else {
		printf("!= v)\n");
	}
}