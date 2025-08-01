#include <arpa/inet.h>
#include <math.h>
#include <netdb.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

// Protocol constants, structs, and macros
#define JOIN_MSG "JOIN\n"
#define NP 3
#define MOD 10289
#define GAMMA1 3
#define GAMMA2 (MOD - 3)
#define GAMMA3 1
#define l 14

enum {
	OP_ADD = 0x01,
	OP_MUL = 0x02,
	OP_CMP = 0x03,
	OP_EQL = 0x04,
	OP_REN = 0x80,
	OP_RES = 0x81
};

typedef struct __attribute__((packed)) {
	uint8_t op;
	uint32_t a;
	uint32_t b;
	int32_t u_shares[l];
	int32_t v_shares[l];
} task_t;

typedef struct __attribute__((packed)) {
	uint8_t op;
	uint32_t value;
} response_t;

int fd = -1;

/*********************************************************************************
 * @brief Look up the host and connect to the specified service.
 * @param const char *host: hostname or IP address
 * @param const char *service: service name or port number
 * @return int: file descriptor for the connected socket, or -1 on error
 *********************************************************************************/
int lookup_and_connect(const char *host, const char *service) {
	struct addrinfo hints;
	struct addrinfo *rp, *result;
	int s;

	/* Translate host name into peer's IP address */
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = 0;
	hints.ai_protocol = 0;

	if ((s = getaddrinfo(host, service, &hints, &result)) != 0) {
		fprintf(stderr, "stream-talk-client: getaddrinfo: %s\n", gai_strerror(s));
		return -1;
	}

	/* Iterate through the address list and try to connect */
	for (rp = result; rp != NULL; rp = rp->ai_next) {
		if ((s = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol)) == -1) {
			continue;
		}

		if (connect(s, rp->ai_addr, rp->ai_addrlen) != -1) {
			break;
		}

		close(s);
	}
	if (rp == NULL) {
		perror("stream-talk-client: connect");
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
 * @brief Run a renormalization operation with the server.
 * @param int32_t value: value to renormalize
 * @return int32_t: renormalized value
 * @note This function sends a request to the server and waits for the response.
 *********************************************************************************/
int32_t runRENORM(int32_t value) {
	response_t r = {OP_REN, htonl((uint32_t)value)};
	if (send(fd, &r, sizeof r, 0) != sizeof r) {
		perror("send");
	}
	if (recvAll(fd, &r, sizeof r) != sizeof r) {
		fprintf(stderr, "Server left\n");
		close(fd);
		fd = -1;
		exit(EXIT_FAILURE);
	} else if (r.op == OP_REN) {
		value = (int32_t)ntohl(r.value);
	} else {
		perror("RENORM did not have proper action code");
		exit(EXIT_FAILURE);
	}
	return value;
}

/*********************************************************************************
 * @brief Main function for the agent that connects to the server and processes tasks.
 * @param int argc: number of command line arguments
 * @param char **argv: command line arguments
 * @return int: exit status
 *********************************************************************************/
int main(int argc, char **argv) {
	if (argc != 3) {
		fprintf(stderr, "usage: %s <server-ip> <port>\n", argv[0]);
		return 1;
	}

	fd = lookup_and_connect(argv[1], argv[2]);
	if (fd < 0) return 1;

	send(fd, JOIN_MSG, 5, 0);
	puts("JOIN sent – waiting for tasks");

	for (;;) {
		task_t t;
		if (recvAll(fd, &t, sizeof t) != sizeof t) break;
		if (t.op == OP_ADD) {
			int32_t x = (int32_t)ntohl(t.a);
			int32_t y = (int32_t)ntohl(t.b);
			int32_t res = (x + y) % MOD;
			response_t r = {OP_RES, htonl((uint32_t)res)};
			if (send(fd, &r, sizeof r, 0) != sizeof r) {
				perror("send");
				exit(EXIT_FAILURE);
			}
		} else if (t.op == OP_MUL) {
			int32_t x = (int32_t)ntohl(t.a);
			int32_t y = (int32_t)ntohl(t.b);
			int32_t res = (x * y) % MOD;
			response_t r = {OP_RES, htonl((uint32_t)res)};
			if (send(fd, &r, sizeof r, 0) != sizeof r) {
				perror("send");
				exit(EXIT_FAILURE);
			}
		} else if (t.op == OP_CMP) {
			int32_t u_shares[l], v_shares[l], eq_share[l], gt_share[l], lt_share[l], prefixEq_share[l], flag_share[l];
			memset(eq_share, 0, sizeof(eq_share));
			memset(gt_share, 0, sizeof(gt_share));
			memset(lt_share, 0, sizeof(lt_share));
			memset(prefixEq_share, 0, sizeof(prefixEq_share));
			memset(flag_share, 0, sizeof(flag_share));
			int32_t tmp_share, tmp2_share, cmp_share = 0;
			for (int32_t i = 0; i < l; i++) {
				u_shares[i] = (int32_t)ntohl(t.u_shares[i]);
				v_shares[i] = (int32_t)ntohl(t.v_shares[i]);
				prefixEq_share[0] = (int32_t)ntohl(t.a);
			}
			for (int32_t j = 0; j < l; j++) {
				tmp_share = (u_shares[j] * v_shares[j]) % MOD;
				tmp_share = runRENORM(tmp_share);
				tmp2_share = (u_shares[j] + v_shares[j] - 2 * tmp_share + MOD) % MOD;
				eq_share[j] = (1 - tmp2_share + MOD) % MOD;
				tmp_share = (1 - v_shares[j] + MOD) % MOD;
				gt_share[j] = (u_shares[j] * tmp_share) % MOD;
				gt_share[j] = runRENORM(gt_share[j]);
				tmp_share = (1 - u_shares[j] + MOD) % MOD;
				lt_share[j] = (tmp_share * v_shares[j]) % MOD;
				lt_share[j] = runRENORM(lt_share[j]);
			}

			for (int32_t j = 1; j < l; j++) {
				tmp_share = (prefixEq_share[j - 1] * eq_share[j - 1]) % MOD;
				prefixEq_share[j] = runRENORM(tmp_share);
			}

			for (int32_t j = 0; j < l; j++) {
				tmp_share = (gt_share[j] - lt_share[j] + MOD) % MOD;
				tmp2_share = (prefixEq_share[j] * tmp_share) % MOD;
				flag_share[j] = runRENORM(tmp2_share);
			}

			for (int32_t j = 0; j < l; j++) {
				cmp_share = (cmp_share + flag_share[j]) % MOD;
			}

			response_t r = {OP_RES, htonl((uint32_t)cmp_share)};
			if (send(fd, &r, sizeof r, 0) != sizeof r) {
				perror("send");
				exit(EXIT_FAILURE);
			}
		}
	}
	puts("Server closed – bye");
	close(fd);
	return 0;
}