#include "../include/init_server.h"
#include "../include/get.h"

int main() {

	setup_server();

	http_get_example("http://localhost:8080/");

	return 0;
}