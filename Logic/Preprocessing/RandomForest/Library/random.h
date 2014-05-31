/**
 * Define random number generator.
 */

#ifndef RANDOM_H
#define RANDOM_H

#include <cstdlib>
#include <time.h>

class Random
{
public:
  Random()
  {
    srand(time(NULL));
  }

  Random(unsigned int seed)
  {
    srand(seed);
  }

  int RandI()
  {
    return rand();
  }

  int RandI(int min, int max)
  {
    return min + rand() % (max-min);
  }

  double RandD()
  {
    return (double) rand() / RAND_MAX;
  }

  double RandD(double min, double max)
  {
    return min + ((double)rand()/RAND_MAX) * (max-min);
  }
};

#endif // RANDOM_H
