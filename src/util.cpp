#include "util.h"
#include <string>
#include <algorithm>


namespace rparser {

int global_seed;
void SetSeed(int seed) { global_seed = seed; }
int GetSeed() { return global_seed; }
void lower(std::string& s) {
	std::transform(s.begin(), s.end(), s.begin(), ::tolower);
}

}