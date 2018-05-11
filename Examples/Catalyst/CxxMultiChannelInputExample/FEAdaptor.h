#ifndef FEADAPTOR_HEADER
#define FEADAPTOR_HEADER

class Attributes;
class Grid;
class Particles;
class vtkCPInputDataDescription;
class vtkCPProcessor;
class vtkPolyData;
class vtkUnstructuredGrid;

class FEAdaptor
{
public:
  FEAdaptor(int numScripts, char* scripts[]);
  ~FEAdaptor();

  void Finalize();

  void CoProcess(Grid& grid, Attributes& attributes, Particles& particles, double time,
    unsigned int timeStep, bool lastTimeStep);

private:
  void BuildVTKVolumetricGrid(Grid& grid, vtkUnstructuredGrid* volumetricGrid);
  void UpdateVTKAttributes(Grid& grid, Attributes& attributes,
    vtkCPInputDataDescription* volumetricGridChannel, vtkUnstructuredGrid* volumetricGrid);
  void BuildVTKVolumetricGridDataStructures(Grid& grid, Attributes& attributes,
    vtkCPInputDataDescription* volumetricGridChannel, vtkUnstructuredGrid* volumetricGrid);
  void BuildVTKParticlesDataStructures(Particles& particles, vtkPolyData* vtkparticles);

  vtkCPProcessor* Processor;
};
#endif
