#ifndef NETINT_H
#define NETINT_H

#include <arpa/inet.h>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <iostream>
#include <memory>
#include <netdb.h>
#include <stdexcept>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

/*********************************************************************************
 * @brief The Detail Namespace Contains Implementation Details That Are Not Needed By The User
 *********************************************************************************/
namespace detail {
	const int MAX_PENDING = 8;
	const char *const JOIN_MSG = "JOIN\n";
	const int NP = 3;
	const int MOD = 10289;
	const int GAMMA1 = 3;
	const int GAMMA2 = (MOD - 3);
	const int GAMMA3 = 1;
	const int l = 14;

	enum OpCodes {
		OP_ADD = 0x01,
		OP_MUL = 0x02,
		OP_CMP = 0x03,
		OP_EQL = 0x04,
		OP_REN = 0x80,
		OP_RES = 0x81
	};

	struct __attribute__((packed)) task_t {
		uint8_t op;
		uint32_t a;
		uint32_t b;
		int32_t u_shares[l];
		int32_t v_shares[l];
	};

	struct __attribute__((packed)) response_t {
		uint8_t op;
		uint32_t value;
	};

	class NetIntContext {
	private:
		/*********************************************************************************
		 * @brief The following variables and helper methods are shared across all NetInt struct instances.
		 *********************************************************************************/
		static std::unique_ptr<NetIntContext> instance;
		int ln = -1;
		int cli[3] = {-1, -1, -1};
		bool initialized = false;

		std::vector<std::string> whitelist;
		bool useWhitelist = false;
		bool showMessages = true;

		NetIntContext() {
			srand(static_cast<unsigned>(time(nullptr)));
		}

		bool isWhitelisted(const std::string &clientIP) const {
			if (!useWhitelist) return true;

			for (const auto &allowedIP : whitelist) {
				if (clientIP == allowedIP) {
					return true;
				}
			}
			return false;
		}

		std::string getClientIP(int clientSocket) const {
			struct sockaddr_in addr;
			socklen_t addrLen = sizeof(addr);

			if (getpeername(clientSocket, (struct sockaddr *)&addr, &addrLen) == 0) {
				char ipStr[INET_ADDRSTRLEN];
				if (inet_ntop(AF_INET, &addr.sin_addr, ipStr, INET_ADDRSTRLEN)) {
					return std::string(ipStr);
				}
			}
			return "unknown";
		}

		/*********************************************************************************
		 * @brief Receive all bytes from a socket until the specified length is reached.
		 * @param int fd: file descriptor of the socket
		 * @param void *buf: buffer to store received data
		 * @param size_t len: number of bytes to receive
		 * @return ssize_t: number of bytes received, or -1 on error
		 *********************************************************************************/
		ssize_t recvAll(int fd, void *buf, size_t len) {
			size_t got = 0;
			while (got < len) {
				ssize_t r = recv(fd, static_cast<char *>(buf) + got, len - got, 0);
				if (r <= 0) return r;
				got += r;
			}
			return got;
		}

		/*********************************************************************************
		 * @brief Bind to a service and listen for incoming connections.
		 * @param const char *service: service name or port number
		 * @return int: socket file descriptor on success, -1 on failure
		 *********************************************************************************/
		int bindAndListen(const std::string &service) {
			struct addrinfo hints;
			struct addrinfo *rp, *result;
			int s;

			memset(&hints, 0, sizeof(hints));
			hints.ai_family = AF_UNSPEC;
			hints.ai_socktype = SOCK_STREAM;
			hints.ai_flags = AI_PASSIVE;
			hints.ai_protocol = 0;

			int err = getaddrinfo(nullptr, service.c_str(), &hints, &result);
			if (err) {
				throw std::runtime_error("getaddrinfo: " + std::string(gai_strerror(err)));
			}

			for (rp = result; rp; rp = rp->ai_next) {
				s = ::socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
				if (s == -1) continue;

				if (bind(s, rp->ai_addr, rp->ai_addrlen) == 0) break;
				close(s);
			}

			if (!rp) {
				freeaddrinfo(result);
				throw std::runtime_error("Could not bind to port");
			}

			if (listen(s, MAX_PENDING)) {
				close(s);
				freeaddrinfo(result);
				throw std::runtime_error("Listen failed");
			}

			freeaddrinfo(result);
			return s;
		}

		/*********************************************************************************
		 * @brief Reconstruct secret from shares using gamma coefficients.
		 * @param int32_t shares[]: array of shares
		 * @return int32_t: reconstructed secret
		 *********************************************************************************/
		int32_t reconstruct(int32_t shares[]) {
			const int32_t gammaArray[3] = {GAMMA1, GAMMA2, GAMMA3};
			int32_t result = 0;
			for (int j = 0; j < 3; j++) {
				result = (result + gammaArray[j] * shares[j]) % MOD;
			}
			return (result < 0) ? result + MOD : result;
		}

		/*********************************************************************************
		 * @brief Renormalize shares to reduce polynomial degree.
		 * @param int32_t shares[]: array of shares to renormalize (modified in-place)
		 * @note Based on Protocol 2.
		 *********************************************************************************/
		void renormalize(int32_t shares[]) {
			const int32_t gammaArray[3] = {GAMMA1, GAMMA2, GAMMA3};
			int32_t rU = rand() % MOD;
			int32_t coeff_r2 = rand() % MOD;
			int32_t share_r[3];
			for (int j = 0; j < 3; j++) {
				share_r[j] = (rU + coeff_r2 * (j + 1)) % MOD;
			}
			int32_t d[3];
			for (int j = 0; j < 3; j++) {
				d[j] = (shares[j] + share_r[j]) % MOD;
			}
			int32_t reshare_d[3][3];
			for (int j = 0; j < 3; j++) {
				int32_t coeff = rand() % MOD;
				for (int k = 0; k < 3; k++) {
					reshare_d[j][k] = (d[j] + coeff * (k + 1)) % MOD;
				}
			}
			for (int k = 0; k < 3; k++) {
				int32_t sum = 0;
				for (int j = 0; j < 3; j++) {
					sum = (sum + gammaArray[j] * reshare_d[j][k]) % MOD;
				}
				shares[k] = (sum - share_r[k]) % MOD;
				if (shares[k] < 0) shares[k] += MOD;
			}
		}

		/*********************************************************************************
		 * @brief Used to split secret shares for agents.
		 * @param int32_t j: party index (0-based)
		 * @param int32_t r: random coefficient
		 * @param int32_t p: secret value
		 * @return int32_t: share for party j
		 *********************************************************************************/
		int32_t split(int32_t j, int32_t r, int32_t p) {
			return (((j + 1) * r + p) + MOD) % MOD;
		}

		/*********************************************************************************
		 * @brief Wait for RENORM responses from all clients and renormalize shares.
		 * @note This function assumes that the clients are already connected and ready to
		 *       receive tasks.
		 *********************************************************************************/
		void waitRenorm() {
			int32_t resultShares[3];
			for (int i = 0; i < 3; ++i) {
				response_t r;
				if (recvAll(cli[i], &r, sizeof(r)) != sizeof(r)) {
					throw std::runtime_error("Client disconnected during RENORM");
				}
				if (r.op != OP_REN) {
					throw std::runtime_error("Invalid RENORM response");
				}
				resultShares[i] = ntohl(r.value);
			}
			renormalize(resultShares);
			for (int i = 0; i < 3; ++i) {
				response_t r = {OP_REN, htonl(static_cast<uint32_t>(resultShares[i]))};
				if (send(cli[i], &r, sizeof(r), 0) != sizeof(r)) {
					throw std::runtime_error("Send failed during RENORM");
				}
			}
		}

		/*********************************************************************************
		 * @brief Run a comparison operation with the MPC protocol.
		 * @param int32_t u: first operand
		 * @param int32_t v: second operand
		 * @return int32_t: result of the comparison
		 *********************************************************************************/
		int32_t runCMP(int32_t u, int32_t v) {
			if (!initialized) throw std::logic_error("MPC context not initialized, need to add the following line before using a NetInt operation:\nestablishPort(\"1234567\");");
			int32_t bits_u[l] = {0};
			int32_t bits_v[l] = {0};
			int32_t uShares[3][l];
			int32_t vShares[3][l];

			for (int32_t i = 0; i < l; i++) {
				bits_u[i] = (u >> (l - 1 - i)) & 1;
				bits_v[i] = (v >> (l - 1 - i)) & 1;
			}

			for (int32_t i = 0; i < l; i++) {
				int32_t ru = rand() % MOD;
				int32_t rv = rand() % MOD;
				for (int j = 0; j < 3; j++) {
					uShares[j][i] = split(j, ru, bits_u[i]);
					vShares[j][i] = split(j, rv, bits_v[i]);
				}
			}

			int r0 = rand() % MOD;
			for (int j = 0; j < 3; j++) {
				task_t t = {OP_CMP, htonl(split(j, r0, 1)), 0, {0}, {0}};
				for (int i = 0; i < l; i++) {
					t.u_shares[i] = htonl(static_cast<int32_t>(uShares[j][i]));
					t.v_shares[i] = htonl(static_cast<int32_t>(vShares[j][i]));
				}
				send(cli[j], &t, sizeof(t), 0);
			}

			for (int32_t j = 0; j < l; j++) {
				waitRenorm();
				waitRenorm();
				waitRenorm();
			}
			for (int32_t j = 1; j < l; j++) {
				waitRenorm();
			}
			for (int32_t j = 0; j < l; j++) {
				waitRenorm();
			}

			int32_t resultShares[3];
			for (int i = 0; i < 3; ++i) {
				response_t r;
				if (recvAll(cli[i], &r, sizeof(r)) != sizeof(r)) {
					throw std::runtime_error("Agent disconnected during CMP");
				}
				resultShares[i] = ntohl(r.value);
			}
			return reconstruct(resultShares);
		}

	public:
		/*********************************************************************************
		 * @brief Get the singleton instance of NetIntContext.
		 * @return NetIntContext&: reference to the singleton instance
		 *********************************************************************************/
		NetIntContext(const NetIntContext &) = delete;
		NetIntContext &operator=(const NetIntContext &) = delete;

		static NetIntContext &getInstance() {
			if (!instance) {
				instance = std::unique_ptr<NetIntContext>(new NetIntContext());
			}
			return *instance;
		}

		/*********************************************************************************
		 * @brief Set an IP whitelist for allowed agent connections.
		 * @param const std::vector<std::string> &allowedIPs: list of allowed IP addresses
		 *********************************************************************************/
		void setIPWhitelist(const std::vector<std::string> &allowedIPs) {
			whitelist = allowedIPs;
			useWhitelist = true;
			printMessage("IP whitelist enabled with " + std::to_string(allowedIPs.size()) + " addresses\n");
		}

		/*********************************************************************************
		 * @brief Clear the IP whitelist, allowing connections from all agents.
		 *********************************************************************************/
		void clearIPWhitelist() {
			whitelist.clear();
			useWhitelist = false;
			printMessage("IP whitelist disabled - allowing all connections\n");
		}

		/*********************************************************************************
		 * @brief Establish a socket connection to the specified port and wait for 3 agent connections.
		 * @param const std::string &port: port number to bind to
		 *********************************************************************************/
		void socket(const std::string &port) {
			if (initialized) return;

			ln = bindAndListen(port);
			char buf[6];
			int joined = 0;

			printMessage("Waiting for 3 agents to connect...\n");
			if (useWhitelist) {
				printMessage("IP whitelist active with " + std::to_string(whitelist.size()) + " allowed addresses\n");
			}

			while (joined < 3) {
				int cfd = accept(ln, nullptr, nullptr);
				if (cfd < 0) {
					perror("accept");
					continue;
				}

				std::string clientIP = getClientIP(cfd);
				if (!isWhitelisted(clientIP)) {
					printMessage("Connection from " + clientIP + " rejected (not in whitelist)\n");
					close(cfd);
					continue;
				}

				memset(buf, 0, sizeof(buf));
				ssize_t r = recv(cfd, buf, sizeof(buf) - 1, 0);
				if (r == 5 && memcmp(buf, "JOIN\n", 5) == 0) {
					cli[joined++] = cfd;
					printMessage("Agent " + std::to_string(joined) + " connected from " + clientIP + "\n");
				} else {
					printMessage("Invalid join message from " + clientIP + "\n");
					close(cfd);
				}
			}
			close(ln);
			ln = -1;
			initialized = true;
			printMessage("All agents connected\n");
		}

		/*********************************************************************************
		 * @brief Disconnect from all agents and clean up resources.
		 *********************************************************************************/
		void disconnect() {
			for (int i = 0; i < 3; i++) {
				if (cli[i] != -1) {
					close(cli[i]);
					cli[i] = -1;
				}
			}
			initialized = false;
			printMessage("Disconnected from all agents\n");
		}

		/*********************************************************************************
		 * @brief Run an addition operation with the MPC protocol.
		 * @param int a: first operand
		 * @param int b: second operand
		 * @return int32_t: result of the addition
		 *********************************************************************************/
		int32_t runAdd(int32_t a, int32_t b) {
			if (!initialized) throw std::logic_error("MPC context not initialized, need to add the following line before using a NetInt operation:\nestablishPort(\"1234567\");");

			int32_t resultShares[3];
			int r1 = rand() % MOD;
			int r2 = rand() % MOD;
			uint8_t op = OP_ADD;

			for (int i = 0; i < 3; i++) {
				int32_t aShare = split(i, r1, a);
				int32_t bShare = split(i, r2, b);
				task_t t = {op, htonl(static_cast<uint32_t>(aShare)), htonl(static_cast<uint32_t>(bShare)), {0}, {0}};
				send(cli[i], &t, sizeof(t), 0);
			}

			for (int i = 0; i < 3; i++) {
				response_t r;
				if (recvAll(cli[i], &r, sizeof(r)) != sizeof(r)) {
					throw std::runtime_error("Agent disconnected during ADD");
				}
				if (r.op != OP_RES) {
					throw std::runtime_error("Invalid ADD response");
				}
				resultShares[i] = ntohl(r.value);
			}
			return reconstruct(resultShares);
		}

		/*********************************************************************************
		 * @brief Control whether non-error messages are printed to console.
		 * @param bool show: true to show messages, false to hide them
		 *********************************************************************************/
		void hideMessages(bool hide = true) {
			showMessages = !hide;
			if (!hide) {
				printMessage("Non-error messages will be printed\n");
			}
		}

		/*********************************************************************************
		 * @brief Run a multiplication operation with the MPC protocol.
		 * @param int a: first operand
		 * @param int b: second operand
		 * @return int32_t: result of the multiplication
		 *********************************************************************************/
		int32_t runMul(int32_t a, int32_t b) {
			if (!initialized) throw std::logic_error("MPC context not initialized, need to add the following line before using a NetInt operation:\nestablishPort(\"1234567\");");

			int32_t resultShares[3];
			int r1 = rand() % MOD;
			int r2 = rand() % MOD;
			uint8_t op = OP_MUL;

			for (int i = 0; i < 3; i++) {
				int32_t aShare = split(i, r1, a);
				int32_t bShare = split(i, r2, b);
				task_t t = {op, htonl(static_cast<uint32_t>(aShare)), htonl(static_cast<uint32_t>(bShare)), {0}, {0}};
				send(cli[i], &t, sizeof(t), 0);
			}

			for (int i = 0; i < 3; i++) {
				response_t r;
				if (recvAll(cli[i], &r, sizeof(r)) != sizeof(r)) {
					throw std::runtime_error("Agent disconnected during MUL");
				}
				if (r.op != OP_RES) {
					throw std::runtime_error("Invalid MUL response");
				}
				resultShares[i] = ntohl(r.value);
			}
			renormalize(resultShares);
			return reconstruct(resultShares);
		}

		/*********************************************************************************
		 * @brief Run a comparison operation with the MPC protocol, specifically the add functions.
		 * @param int32_t a: first operand
		 * @param int32_t b: second operand
		 * @return int32_t: result of the comparison (0 or 1)
		 *********************************************************************************/
		int32_t runSub(int32_t a, int32_t b) {
			// a - b = a + (MOD - b) mod MOD
			return runAdd(a, (MOD - b) % MOD);
		}

		/*********************************************************************************
		 * @brief Run less than comparison with the MPC protocol, specifically.
		 * @param int32_t a: first operand
		 * @param int32_t b: second operand
		 * @return int32_t: result of the comparison (0 or 1)
		 *********************************************************************************/
		int32_t runLT(int32_t a, int32_t b) {
			int32_t cmp = runCMP(a, b);
			return (cmp > MOD / 2) ? 1 : 0;
		}

		/*********************************************************************************
		 * @brief Run less than or equal to comparison with the MPC protocol.
		 * @param int32_t a: first operand
		 * @param int32_t b: second operand
		 * @return int32_t: result of the comparison (0 or 1)
		 *********************************************************************************/
		int32_t runLE(int32_t a, int32_t b) {
			int32_t cmp = runCMP(a, b);
			return (cmp == 0 || cmp > MOD / 2) ? 1 : 0;
		}

		/*********************************************************************************
		 * @brief Run greater than comparison with the MPC protocol.
		 * @param int32_t a: first operand
		 * @param int32_t b: second operand
		 * @return int32_t: result of the comparison (0 or 1)
		 *********************************************************************************/
		int32_t runGT(int32_t a, int32_t b) {
			return runLT(b, a);
		}

		/*********************************************************************************
		 * @brief Run greater than or equal to comparison with the MPC protocol.
		 * @param int32_t a: first operand
		 * @param int32_t b: second operand
		 * @return int32_t: result of the comparison (0 or 1)
		 *********************************************************************************/
		int32_t runGE(int32_t a, int32_t b) {
			return runLE(b, a);
		}

		/*********************************************************************************
		 * @brief Run equality comparison with the MPC protocol.
		 * @param int32_t a: first operand
		 * @param int32_t b: second operand
		 * @return int32_t: result of the comparison (0 or 1)
		 *********************************************************************************/
		int32_t runEQ(int32_t a, int32_t b) {
			int32_t cmp = runCMP(a, b);
			return (cmp == 0) ? 1 : 0;
		}

		/*********************************************************************************
		 * @brief Run not equal to comparison with the MPC protocol.
		 * @param int32_t a: first operand
		 * @param int32_t b: second operand
		 * @return int32_t: result of the comparison (0 or 1)
		 *********************************************************************************/
		int32_t runNE(int32_t a, int32_t b) {
			return 1 - runEQ(a, b);
		}

	private:
		void printMessage(const std::string &msg) const {
			if (showMessages) {
				std::cout << msg;
			}
		}
	};

	std::unique_ptr<NetIntContext> NetIntContext::instance;
}

/*********************************************************************************
 * @brief Public API for the NetInt library. Functions and structures beyond this point are intended to be interacted with.
 * This provides convenience functions to establish connections, disconnect agents,
 * and manage IP whitelists.
 *********************************************************************************/
namespace detail {
	class NetIntContext;
}
void establishPort(const std::string &port);
void disconnectAgents();
void setWhitelist(const std::vector<std::string> &allowedIPs);
void clearWhitelist();
void hideMessages(bool hide = true);

inline void establishPort(const std::string &port) {
	detail::NetIntContext::getInstance().socket(port);
}

inline void disconnectAgents() {
	detail::NetIntContext::getInstance().disconnect();
}

inline void setWhitelist(const std::vector<std::string> &allowedIPs) {
	detail::NetIntContext::getInstance().setIPWhitelist(allowedIPs);
}

inline void clearWhitelist() {
	detail::NetIntContext::getInstance().clearIPWhitelist();
}

inline void hideMessages(bool hide) {
	detail::NetIntContext::getInstance().hideMessages(hide);
}

struct NetInt {
	int32_t value;

	NetInt(int32_t val = 0) : value(val) {}
	NetInt(const NetInt &other) = default;
	NetInt &operator=(const NetInt &other) = default;
	NetInt &operator=(int32_t val) {
		value = val;
		return *this;
	}

	// Disable division, modulo, and bitwise operators
	NetInt operator/(const NetInt &) const = delete;
	NetInt operator/(int32_t) const = delete;
	NetInt &operator/=(const NetInt &) = delete;
	NetInt &operator/=(int32_t) = delete;

	NetInt operator%(const NetInt &) const = delete;
	NetInt operator%(int32_t) const = delete;
	NetInt &operator%=(const NetInt &) = delete;
	NetInt &operator%=(int32_t) = delete;

	NetInt operator&(const NetInt &other) const = delete;
	NetInt operator|(const NetInt &other) const = delete;
	NetInt operator^(const NetInt &other) const = delete;
	NetInt operator~() const = delete;

	friend void swap(NetInt &a, NetInt &b) noexcept {
		std::swap(a.value, b.value);
	}

	// Working Arithmetic operators
	template <typename T>
	NetInt operator+(const T &other) const {
		return NetInt(detail::NetIntContext::getInstance().runAdd(value, static_cast<int32_t>(other)));
	}
	template <typename T>
	NetInt operator-(const T &other) const {
		return NetInt(detail::NetIntContext::getInstance().runSub(value, static_cast<int32_t>(other)));
	}
	template <typename T>
	NetInt operator*(const T &other) const {
		return NetInt(detail::NetIntContext::getInstance().runMul(value, static_cast<int32_t>(other)));
	}

	// Compound assignment operators
	template <typename T>
	NetInt &operator+=(const T &other) {
		value = detail::NetIntContext::getInstance().runAdd(value, static_cast<int32_t>(other));
		return *this;
	}
	template <typename T>
	NetInt &operator-=(const T &other) {
		value = detail::NetIntContext::getInstance().runSub(value, static_cast<int32_t>(other));
		return *this;
	}
	template <typename T>
	NetInt &operator*=(const T &other) {
		value = detail::NetIntContext::getInstance().runMul(value, static_cast<int32_t>(other));
		return *this;
	}

	// Comparison operators
	template <typename T>
	bool operator<(const T &other) const {
		return detail::NetIntContext::getInstance().runLT(value, static_cast<int32_t>(other)) == 1;
	}
	template <typename T>
	bool operator<=(const T &other) const {
		return detail::NetIntContext::getInstance().runLE(value, static_cast<int32_t>(other)) == 1;
	}
	template <typename T>
	bool operator>(const T &other) const {
		return detail::NetIntContext::getInstance().runGT(value, static_cast<int32_t>(other)) == 1;
	}
	template <typename T>
	bool operator>=(const T &other) const {
		return detail::NetIntContext::getInstance().runGE(value, static_cast<int32_t>(other)) == 1;
	}
	template <typename T>
	bool operator==(const T &other) const {
		return detail::NetIntContext::getInstance().runEQ(value, static_cast<int32_t>(other)) == 1;
	}
	template <typename T>
	bool operator!=(const T &other) const {
		return detail::NetIntContext::getInstance().runNE(value, static_cast<int32_t>(other)) == 1;
	}

	NetInt &operator++() {
		value = detail::NetIntContext::getInstance().runAdd(value, 1);
		return *this;
	}
	NetInt operator++(int) {
		NetInt temp = *this;
		value = detail::NetIntContext::getInstance().runAdd(value, 1);
		return temp;
	}
	NetInt &operator--() {
		value = detail::NetIntContext::getInstance().runSub(value, 1);
		return *this;
	}
	NetInt operator--(int) {
		NetInt temp = *this;
		value = detail::NetIntContext::getInstance().runSub(value, 1);
		return temp;
	}
	NetInt operator-() const {
		return NetInt((detail::MOD - value) % detail::MOD);
	}
	friend std::ostream &operator<<(std::ostream &os, const NetInt &netint) {
		return os << netint.value;
	}
	friend std::istream &operator>>(std::istream &is, NetInt &netint) {
		return is >> netint.value;
	}

	operator int32_t() const { return value; }
	int32_t getVal() const { return value; }
};

// Non-member arithmetic for int on the left
template <typename T>
NetInt operator+(const T &lhs, const NetInt &rhs) {
	return NetInt(detail::NetIntContext::getInstance().runAdd(static_cast<int32_t>(lhs), rhs.value));
}
template <typename T>
NetInt operator-(const T &lhs, const NetInt &rhs) {
	return NetInt(detail::NetIntContext::getInstance().runSub(static_cast<int32_t>(lhs), rhs.value));
}
template <typename T>
NetInt operator*(const T &lhs, const NetInt &rhs) {
	return NetInt(detail::NetIntContext::getInstance().runMul(static_cast<int32_t>(lhs), rhs.value));
}

// Non-member comparisons for int on the left
template <typename T>
bool operator<(const T &lhs, const NetInt &rhs) {
	return detail::NetIntContext::getInstance().runLT(static_cast<int32_t>(lhs), rhs.value) == 1;
}
template <typename T>
bool operator<=(const T &lhs, const NetInt &rhs) {
	return detail::NetIntContext::getInstance().runLE(static_cast<int32_t>(lhs), rhs.value) == 1;
}
template <typename T>
bool operator>(const T &lhs, const NetInt &rhs) {
	return detail::NetIntContext::getInstance().runGT(static_cast<int32_t>(lhs), rhs.value) == 1;
}
template <typename T>
bool operator>=(const T &lhs, const NetInt &rhs) {
	return detail::NetIntContext::getInstance().runGE(static_cast<int32_t>(lhs), rhs.value) == 1;
}
template <typename T>
bool operator==(const T &lhs, const NetInt &rhs) {
	return detail::NetIntContext::getInstance().runEQ(static_cast<int32_t>(lhs), rhs.value) == 1;
}
template <typename T>
bool operator!=(const T &lhs, const NetInt &rhs) {
	return detail::NetIntContext::getInstance().runNE(static_cast<int32_t>(lhs), rhs.value) == 1;
}

#endif // NETINT_H