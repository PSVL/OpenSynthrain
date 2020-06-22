#include "rng.h"
#include <array>
#include <iterator>
#include <algorithm>
#include <random>
#include <sstream>
#include <string>
#include <functional>

std::mt19937 RNG::mt;

namespace RNG
{
	static class InitRNG
	{
	public:
		InitRNG()
		{
			std::random_device rd;
			std::array<int, std::mt19937::state_size> seed_data;
			std::generate_n(seed_data.data(), seed_data.size(), std::ref(rd));
			std::seed_seq seq(std::begin(seed_data), std::end(seed_data));
			RNG::mt.seed(seq);
		}
	} init;
}
