#ifndef __vtkPSciVizPCAStats_h
#define __vtkPSciVizPCAStats_h

#include "vtkSciVizStatistics.h"

class vtkPSciVizPCAStats : public vtkSciVizStatistics
{
public:
  static vtkPSciVizPCAStats* New();
  vtkTypeRevisionMacro(vtkPSciVizPCAStats,vtkSciVizStatistics);
  virtual void PrintSelf( ostream& os, vtkIndent indent );

  vtkSetMacro(NormalizationScheme,int);
  vtkGetMacro(NormalizationScheme,int);

  vtkSetMacro(BasisScheme,int);
  vtkGetMacro(BasisScheme,int);

  vtkSetMacro(FixedBasisSize,int);
  vtkGetMacro(FixedBasisSize,int);

  vtkSetClampMacro(FixedBasisEnergy,double,0.,1.);
  vtkGetMacro(FixedBasisEnergy,double);

protected:
  vtkPSciVizPCAStats();
  virtual ~vtkPSciVizPCAStats();

  virtual const char* GetModelDataTypeName() { return "vtkMultiBlockDataSet"; }
  virtual int RequestModelDataObject( vtkInformation* outInfo );

  virtual int FitModel( vtkDataObject*& model, vtkInformationVector* output, vtkTable* trainingData );
  virtual int AssessData( vtkTable* observations, vtkDataObject* dataset, vtkDataObject* model );

  int NormalizationScheme;
  int BasisScheme;
  int FixedBasisSize;
  double FixedBasisEnergy;

private:
  vtkPSciVizPCAStats( const vtkPSciVizPCAStats& ); // Not implemented.
  void operator = ( const vtkPSciVizPCAStats& ); // Not implemented.
};

#endif // __vtkPSciVizPCAStats_h
