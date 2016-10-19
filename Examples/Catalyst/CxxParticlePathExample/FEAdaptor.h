#ifndef FEADAPTOR_HEADER
#define FEADAPTOR_HEADER

class Attributes;
class Grid;

#include <string>
#include <vector>

namespace FEAdaptor
{
void Initialize(std::vector<std::string>& scripts);

void Finalize();

void CoProcess(
  Grid& grid, Attributes& attributes, double time, unsigned int timeStep, bool lastTimeStep);
}

#endif
