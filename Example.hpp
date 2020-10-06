#ifndef EXAMPLE_HPP
#define EXAMPLE_HPP

#include <string>
#include <iostream>

namespace {
	enum Foods {
		Pizza,
		Pasta,
		ScrambledEggs,
		Cake
	};
}

namespace Nature {
	enum class Animals {
		Parrots,
		Cat,
		Dogs,
		Monkeys,
	};
}

namespace          City      {
	enum class FastFoodChains {
		McDonalds,
		KFC,
		TacoBell,
		Subway,
		ChickFilA,
	};
}

/*
	This is a multiple line comment that
	should be cleaned by the sanitizer
*/

#endif