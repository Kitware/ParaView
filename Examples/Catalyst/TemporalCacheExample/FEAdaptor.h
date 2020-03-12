#ifndef FEADAPTOR_HEADER
#define FEADAPTOR_HEADER

// Description:
// Functions that the simulation calls to setup, populate, and execute catalyst.
//
// In this example the key things to know about are the way that the temporal caching
// feature in Catalyst is turned on and fed. See the code with "KEY POINT" comments.
//
// Besides functioning as an example of set up a temporal capable adaptor,
// there is also vtkCPTestPipeline which is sample C++ CatalystPipeline that demonstrates temporal
// filters.

class Attributes;
class Grid;

namespace FEAdaptor
{
void Initialize(int numScripts, char* scripts[]);

void CoProcess(
  Grid& grid, Attributes& attributes, double time, unsigned int timeStep, bool lastTimeStep);

void Finalize();
}

#endif
