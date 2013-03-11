#ifndef FEADAPTOR_HEADER
#define FEADAPTOR_HEADER

class Attributes;
class Grid;

namespace Catalyst
{
  void Initialize(int numScripts, const char* scripts[]);

  void Finalize();

  void CoProcess(Grid& grid, Attributes& attributes, double time, unsigned int timeStep);
}

#endif
