#ifndef FEADAPTOR_HEADER
#define FEADAPTOR_HEADER

class vtkCPProcessor;

class FEAdaptor
{
public:
  FEAdaptor(int numScripts, char* scripts[]);
  ~FEAdaptor();

  void Finalize();

  void CoProcess(double time, unsigned int timeStep, bool lastTimeStep);

private:
  vtkCPProcessor* Processor;
};
#endif
