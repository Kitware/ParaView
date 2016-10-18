#ifndef FEADAPTOR_HEADER
#define FEADAPTOR_HEADER

#include <mpi.h>

class Attributes;
class Grid;

namespace FEAdaptor
{
void Initialize(int numScripts, char* scripts[], MPI_Comm* comm);

void Finalize();

void CoProcess(
  Grid& grid, Attributes& attributes, double time, unsigned int timeStep, bool lastTimeStep);
}

#endif
