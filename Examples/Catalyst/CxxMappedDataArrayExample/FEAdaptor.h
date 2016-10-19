#ifndef FEADAPTOR_HEADER
#define FEADAPTOR_HEADER

class Attributes;
class Grid;

namespace FEAdaptor
{
void Initialize(int numScripts, char* scripts[]);

void Finalize();

void CoProcess(
  Grid& grid, Attributes& attributes, double time, unsigned int timeStep, bool lastTimeStep);
}

#endif
