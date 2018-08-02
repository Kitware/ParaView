#ifndef FEADAPTOR_HEADER
#define FEADAPTOR_HEADER

class vtkCPProcessor;
class vtkHyperTreeGrid;
class vtkHyperTreeCursor;

class FEAdaptor
{
public:
  FEAdaptor(int numScripts, char* scripts[]);
  ~FEAdaptor();

  void Finalize();

  void CoProcess(double time, unsigned int timeStep, bool lastTimeStep);

private:
  void AddData(vtkHyperTreeGrid* htg, vtkHyperTreeCursor* cursor);
  bool ShouldRefine(unsigned int level);
  void SubdivideLeaves(vtkHyperTreeGrid* htg, vtkHyperTreeCursor* cursor, long long treeId);
  void FillHTG(vtkHyperTreeGrid*);

  vtkCPProcessor* Processor;
};
#endif
