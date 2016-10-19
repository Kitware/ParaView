#ifndef FEADAPTOR_HEADER
#define FEADAPTOR_HEADER

#include <string>

class Attributes;
class Grid;

namespace FEAdaptor
{
void Initialize(int outputFrequency, std::string fileName);

void Finalize();

void CoProcess(
  Grid& grid, Attributes& attributes, double time, unsigned int timeStep, bool lastTimeStep);
}

#endif
