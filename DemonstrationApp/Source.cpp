#include "dbapi.h"
#include <iostream>
#include <string>

int main() {
	// Vytvoření db
	Db* db = Db::open("testdb");

	std::cin.get();
	return 0;
}
