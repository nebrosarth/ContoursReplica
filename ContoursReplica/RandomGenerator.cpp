#include "RandomGenerator.h"

RandomGenerator& RandomGenerator::instance()
{
	static RandomGenerator generator;
	return generator;
}

QPoint RandomGenerator::getRandomPoint(int maxWidth, int maxHeight)
{
	return { getRandomInt(maxWidth), getRandomInt(maxHeight) };
}

QColor RandomGenerator::getRandomColor()
{
	return QColor( getRandomInt(255), getRandomInt(255), getRandomInt(255));
}

int RandomGenerator::getRandomInt(int max)
{
	return distribution(rng) * max;
}

int RandomGenerator::getRandomInt(int min, int max)
{
	if (min == max) return max;
	std::uniform_int_distribution<int> dist(min, max - 1);
	return dist(rng);
}

float RandomGenerator::getRandomFloat(float min, float max)
{
	std::uniform_real_distribution<float> dist(min, max);
	return dist(rng);
}

RandomGenerator::RandomGenerator() :
	rng(dev()), distribution(0.0, 1.0)
{
}
