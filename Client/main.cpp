#include "Client.h"

int main() {
	
	auto app = new Client();
	app->run("AIE", 1280, 720, false);
	delete app;

	return 0;
}