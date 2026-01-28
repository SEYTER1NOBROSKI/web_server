#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>
#include <stdatomic.h>

#define TARGET_IP "127.0.0.1"
#define TARGET_PORT 8080
#define NUM_THREADS 500
#define REQUESTS_PER_THREAD 10 

atomic_int success_count = 0;
atomic_int fail_count = 0;
atomic_int bytes_received = 0;

const char *request = 
	"GET /index.html HTTP/1.1\r\n"
	"Host: localhost\r\n"
	"Connection: keep-alive\r\n"
	"\r\n";

void *worker_thread(void *arg) {
	int sock;
	struct sockaddr_in server_addr;
	char buffer[4096];

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		atomic_fetch_add(&fail_count, REQUESTS_PER_THREAD);
		return NULL;
	}

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(TARGET_PORT);
	if (inet_pton(AF_INET, TARGET_IP, &server_addr.sin_addr) <= 0) {
		close(sock);
		return NULL;
	}

	if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
		atomic_fetch_add(&fail_count, REQUESTS_PER_THREAD);
		close(sock);
		return NULL;
	}

	for (int i = 0; i < REQUESTS_PER_THREAD; i++) {
		if (send(sock, request, strlen(request), 0) < 0) {
			atomic_fetch_add(&fail_count, 1);
			break;
	}

		ssize_t len = recv(sock, buffer, sizeof(buffer) - 1, 0);
		if (len > 0) {
			atomic_fetch_add(&success_count, 1);
			atomic_fetch_add(&bytes_received, len);
		} else {
			atomic_fetch_add(&fail_count, 1);
			break; 
		}

		usleep(10000);
	}

	close(sock);
	return NULL;
}

int main() {
	pthread_t threads[NUM_THREADS];
	printf("==========================================\n");
	printf("Starting Benchmark (Load Test)\n");
	printf("Target: %s:%d\n", TARGET_IP, TARGET_PORT);
	printf("Threads (Clients): %d\n", NUM_THREADS);
	printf("Requests per Client: %d\n", REQUESTS_PER_THREAD);
	printf("Total expected requests: %d\n", NUM_THREADS * REQUESTS_PER_THREAD);
	printf("==========================================\n");

	struct timespec start, end;
	clock_gettime(CLOCK_MONOTONIC, &start);

	for (int i = 0; i < NUM_THREADS; i++) {
		if (pthread_create(&threads[i], NULL, worker_thread, NULL) != 0) {
			perror("Failed to create thread");
		}
		usleep(2000);
	}

	for (int i = 0; i < NUM_THREADS; i++) {
		pthread_join(threads[i], NULL);
	}

	clock_gettime(CLOCK_MONOTONIC, &end);

	double time_taken = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
	int total_requests = success_count + fail_count;
	double rps = success_count / time_taken;

	printf("\n--- TEST REPORT ---\n");
	if (fail_count == 0) {
		printf("RESULT: \033[32mSUCCESS\033[0m\n");
	} else {
		printf("RESULT: \033[31mPASSED WITH ERRORS\033[0m\n");
	}

	printf("Time taken:        %.2f seconds\n", time_taken);
	printf("Total requests:    %d\n", total_requests);
	printf("Successful hits:   %d\n", atomic_load(&success_count));
	printf("Failed hits:       %d\n", atomic_load(&fail_count));
	printf("Total bytes read:  %d MB\n", atomic_load(&bytes_received) / 1024 / 1024);
	printf("---------------------------\n");
	printf("Requests Per Sec:  \033[1m%.2f req/sec\033[0m\n", rps);
	printf("==========================================\n");

	return 0;
}