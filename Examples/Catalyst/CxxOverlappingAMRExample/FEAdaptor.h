#ifndef FEADAPTOR_HEADER
#define FEADAPTOR_HEADER

namespace FEAdaptor
{
void Initialize(int numScripts, char* scripts[]);

void Finalize();

void CoProcess(double time, unsigned int timeStep, bool lastTimeStep);
}

#endif
