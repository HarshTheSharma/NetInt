#include "common.h"
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#define JOIN_MSG "JOIN\n"
#define MAX_PENDING 8
int cli[NP] = {-1, -1, -1};

int32_t gammaArray[NP] = {GAMMA1, GAMMA2, GAMMA3}; // Coefficients for reconstruction

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

/*********************************************************************************
 * @brief Bind to a service and listen for incoming connections.
 * @param const char *service: service name or port number
 * @return int: socket file descriptor on success, -1 on failure
 *********************************************************************************/
int bind_and_listen(const char *service) {
	struct addrinfo hints;
	struct addrinfo *rp, *result;
	int s;

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	hints.ai_protocol = 0;

	if ((s = getaddrinfo(NULL, service, &hints, &result)) != 0) {
		fprintf(stderr, "stream-talk-server: getaddrinfo: %s\n", gai_strerror(s));
		return -1;
	}

	for (rp = result; rp != NULL; rp = rp->ai_next) {
		if ((s = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol)) == -1) {
			continue;
		}

		if (!bind(s, rp->ai_addr, rp->ai_addrlen)) {
			break;
		}

		close(s);
	}
	if (rp == NULL) {
		perror("stream-talk-server: bind");
		return -1;
	}
	if (listen(s, MAX_PENDING) == -1) {
		perror("stream-talk-server: listen");
		close(s);
		return -1;
	}
	freeaddrinfo(result);

	return s;
}

/*********************************************************************************
 * @brief Receive all bytes from a socket until the specified length is reached.
 * @param int fd: file descriptor of the socket
 * @param void *buf: buffer to store received data
 * @param size_t len: number of bytes to receive
 * @return ssize_t: number of bytes received, or -1 on error
 *********************************************************************************/
static ssize_t recvAll(int fd, void *buf, size_t len) {
	size_t got = 0;
	while (got < len) {
		ssize_t r = recv(fd, (char *)buf + got, len - got, 0);
		if (r <= 0) return r;
		got += (size_t)r;
	}
	return (ssize_t)got;
}

/*********************************************************************************
 * @brief Wait for RENORM responses from all clients and renormalize shares.
 * @note This function assumes that the clients are already connected and ready to
 *       receive tasks.
 *********************************************************************************/
void waitRENORM() {
	int32_t resultShares[NP];
	for (int i = 0; i < NP; ++i) {
		response_t r;
		if (recvAll(cli[i], &r, sizeof r) != sizeof r) {
			fprintf(stderr, "Client %d left\n", i + 1);
			close(cli[i]);
			cli[i] = -1;
			exit(EXIT_FAILURE);
		} else if (r.op == OP_REN) {
			resultShares[i] = (int32_t)ntohl(r.value);
		} else {
			perror("RENORM did not have proper action code");
			exit(EXIT_FAILURE);
		}
	}
	RENORMALIZE(resultShares);
	for (int i = 0; i < NP; ++i) {
		response_t r = {OP_REN, htonl((uint32_t)resultShares[i])};
		if (send(cli[i], &r, sizeof r, 0) != sizeof r) {
			perror("send");
			break;
		}
	}
}

/*********************************************************************************
 * @brief Run an addition operation with the MPC protocol.
 * @param int a: first operand
 * @param int b: second operand
 * @return int32_t: result of the addition
 *********************************************************************************/
int32_t runADD(int a, int b) {
	int32_t resultShares[NP];
	int r1 = rand() % MOD;
	int r2 = rand() % MOD;
	int8_t op = OP_ADD;
	for (int i = 0; i < NP; ++i) {
		int32_t a_share = SPLIT(i, r1, a);
		int32_t b_share = SPLIT(i, r2, b);
		task_t t = {
			.op = op,
			.a = htonl((uint32_t)a_share),
			.b = htonl((uint32_t)b_share),
			.u_shares = {0},
			.v_shares = {0}};
		send(cli[i], &t, sizeof t, 0);
	}
	printf("Sent task %d + %d\n", a, b);
	fflush(stdout);
	for (int i = 0; i < NP; ++i) {
		response_t r;
		if (recvAll(cli[i], &r, sizeof r) != sizeof r) {
			fprintf(stderr, "Client %d left\n", i + 1);
			close(cli[i]);
			cli[i] = -1;
			exit(EXIT_FAILURE);
		} else if (r.op == OP_RES)
			resultShares[i] = (int32_t)ntohl(r.value);
	}
	return RECONSTRUCT(resultShares);
}

/*********************************************************************************
 * @brief Run a multiplication operation with the MPC protocol.
 * @param int a: first operand
 * @param int b: second operand
 * @return int32_t: result of the multiplication
 *********************************************************************************/
int32_t runMUL(int a, int b) {
	int32_t resultShares[NP];
	int r1 = rand() % MOD;
	int r2 = rand() % MOD;
	int8_t op = OP_MUL;
	for (int i = 0; i < NP; ++i) {
		int32_t a_share = SPLIT(i, r1, a);
		int32_t b_share = SPLIT(i, r2, b);
		task_t t = {
			.op = op,
			.a = htonl((uint32_t)a_share),
			.b = htonl((uint32_t)b_share),
			.u_shares = {0},
			.v_shares = {0}};
		send(cli[i], &t, sizeof t, 0);
	}
	printf("Sent task %d * %d\n", a, b);
	fflush(stdout);
	for (int i = 0; i < NP; ++i) {
		response_t r;
		if (recvAll(cli[i], &r, sizeof r) != sizeof r) {
			fprintf(stderr, "Client %d left\n", i + 1);
			close(cli[i]);
			cli[i] = -1;
			exit(EXIT_FAILURE);
		} else if (r.op == OP_RES) {
			resultShares[i] = (int32_t)ntohl(r.value);
		}
	}
	RENORMALIZE(resultShares);
	return RECONSTRUCT(resultShares);
}

int32_t runCMP(int32_t u, int32_t v, int32_t *c, int32_t *e) {
	int32_t bits_u[l];
	int32_t bits_v[l];
	int32_t u_shares[NP][l];
	int32_t v_shares[NP][l];
	for (int32_t i = 0; i < l; i++) {
		bits_u[i] = 0;
		bits_v[i] = 0;
	}
	for (int32_t i = 0; i < l; i++) {
		bits_u[i] = (u >> (l - 1 - i)) & 1;
		bits_v[i] = (v >> (l - 1 - i)) & 1;
	}
	for (int32_t i = 0; i < l; i++) {
		int32_t ru = rand() % MOD;
		int32_t rv = rand() % MOD;
		for (int32_t j = 0; j < NP; j++) {
			u_shares[j][i] = htonl(SPLIT(j, ru, bits_u[i]));
			v_shares[j][i] = htonl(SPLIT(j, rv, bits_v[i]));
		}
	}
	int r0 = rand() % MOD;
	for (int32_t j = 0; j < NP; j++) {
		task_t t = {
			.op = OP_CMP,
			.a = htonl(SPLIT(j, r0, 1)),
			.b = htonl((uint32_t)0),
			.u_shares = {0},
			.v_shares = {0}};
		memcpy(t.u_shares, u_shares[j], sizeof(t.u_shares));
		memcpy(t.v_shares, v_shares[j], sizeof(t.v_shares));
		send(cli[j], &t, sizeof t, 0);
	}

	for (int32_t j = 0; j < l; j++) {
		waitRENORM();
		waitRENORM();
		waitRENORM();
	}
	for (int32_t j = 1; j < l; j++) {
		waitRENORM();
	}
	for (int32_t j = 0; j < l; j++) {
		waitRENORM();
	}
	int32_t resultShares[NP];
	for (int i = 0; i < NP; ++i) {
		response_t r;
		if (recvAll(cli[i], &r, sizeof r) != sizeof r) {
			fprintf(stderr, "Client %d left\n", i + 1);
			close(cli[i]);
			cli[i] = -1;
			exit(EXIT_FAILURE);
		} else if (r.op == OP_RES) {
			resultShares[i] = (int32_t)ntohl(r.value);
		}
	}
	int32_t cmp = RECONSTRUCT(resultShares);
	if (cmp >= 0 && cmp <= (MOD / 2))
		*c = 1;
	else
		*c = 0;

	if (cmp == 0)
		*e = 1;
	else
		*e = 0;
	return 1;
}

int main(int argc, char **argv) {
	if (argc != 2) {
		fprintf(stderr, "usage: %s <port>\n", argv[0]);
		return 1;
	}
	srand((unsigned)time(NULL));

	int ln = bind_and_listen(argv[1]);
	if (ln < 0) return 1;
	char line[100], opStr[3] = {0};
	int joined = 0;
	char buf[8];

	printf("Waiting for %d JOINs on port %s …\n", NP, argv[1]);

	// HANDSHAKE
	while (joined < NP) {
		int cfd = accept(ln, NULL, NULL);
		if (cfd < 0) {
			perror("accept");
			continue;
		}

		memset(buf, 0, sizeof(buf));
		ssize_t r = recv(cfd, buf, sizeof buf, 0);
		if (r == (ssize_t)strlen(JOIN_MSG) && memcmp(buf, JOIN_MSG, r) == 0) {
			cli[joined++] = cfd;
			printf("Client %d joined\n", joined);
		} else {
			close(cfd);
		}
	}
	puts("All clients joined – starting loop");

	for (int tick = 1;; ++tick) {
		// take input here from file or console;
		int a, b;
		int32_t c = 0, e = 0, result = 0;
		printf("Enter an expression (e.g., a + b, a <= b): ");
		if (!fgets(line, sizeof(line), stdin)) {
			fprintf(stderr, "Input error.\n");
			return 1;
		}
		if (sscanf(line, "%d %2s %d", &a, opStr, &b) != 3) {
			fprintf(stderr, "Invalid format. Expected: a op b\n");
			continue;
		}
		if (strcmp(opStr, "+") == 0) {
			result = runADD(a, b);
		} else if (strcmp(opStr, "-") == 0) {
			runCMP(a, b, &c, &e);
			if (!c) {
				result = -1 * runADD(-a, b);
			} else {
				result = runADD(a, -b);
			}
		} else if (strcmp(opStr, "*") == 0) {
			result = runMUL(a, b);
		} else if (strcmp(opStr, "<") == 0) {
			runCMP(a, b, &c, &e);
			if (!c) result = 1;
		} else if (strcmp(opStr, "<=") == 0) {
			runCMP(b, a, &c, &e);
			if (c) result = 1;
		} else if (strcmp(opStr, ">") == 0) {
			runCMP(b, a, &c, &e);
			if (!c) result = 1;
		} else if (strcmp(opStr, ">=") == 0) {
			runCMP(a, b, &c, &e);
			if (c) result = 1;
		} else if (strcmp(opStr, "==") == 0) {
			runCMP(a, b, &c, &e);
			if (e)
				result = 1;
			else
				result = 0;
		} else if (strcmp(opStr, "!=") == 0) {
			runCMP(a, b, &c, &e);
			if (!e) result = 1;
		} else {
			fprintf(stderr, "Unsupported operator: %s\n", opStr);
			continue;
		}

		printf("Result: %d\n", result);
	}
}