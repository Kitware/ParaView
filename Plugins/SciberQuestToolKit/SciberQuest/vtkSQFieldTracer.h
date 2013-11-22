/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/
// .NAME vtkSQFieldTracer - Streamline generator
// .SECTION Description
//
// Scalable field line tracer using RK45 Adds capability to
// terminate trace upon intersection with one of a set of
// surfaces.
// TODO verify that VTK rk45 implementation increases step size!!

#ifndef __vtkSQFieldTracer_h
#define __vtkSQFieldTracer_h

#include "vtkSciberQuestModule.h" // for export macro
#include "vtkDataSetAlgorithm.h"

class vtkUnstructuredGrid;
class vtkSQOOCReader;
class vtkMultiProcessController;
class vtkInitialValueProblemSolver;
class vtkPointSet;
class vtkPVXMLElement;
//BTX
class IdBlock;
class FieldLine;
class FieldTraceData;
class TerminationCondition;
//ETX


class VTKSCIBERQUEST_EXPORT vtkSQFieldTracer : public vtkDataSetAlgorithm
{
public:
  vtkTypeMacro(vtkSQFieldTracer,vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkSQFieldTracer *New();

  // Description:
  // Specify the dataset with the vector field to analyze.
  void AddVectorInputConnection(vtkAlgorithmOutput* algOutput);
  void ClearVectorInputConnections();
  // Description:
  // Specify a set of seed points to use.
  void AddSeedPointInputConnection(vtkAlgorithmOutput* algOutput);
  void ClearSeedPointInputConnections();
  // Description:
  // Specify a set of surfaces to use.
  void AddTerminatorInputConnection(vtkAlgorithmOutput* algOutput);
  void ClearTerminatorInputConnections();

  // Description:
  // Initialize the object from an xml document.
  int Initialize(vtkPVXMLElement *root);

  // Description:
  // Set the run mode of the filter.
  // The enumeration is as follows:
  //    1  STREAM   filter produces stream lines
  //    2  TOPOLOGY filter produces field topology map
  //    3  POINCARE filter produces a displacement map
  //    4  DISPLACEMENT filter produces a poincare map
  // This allows this filter to serve as multiple ParaView filters,
  // the OOCFieldTracer, OOCDTopologyMapper, and OOCDPoincareMapper
  // NOTE This only works if Mode is set before the filter runs.
  // PV gets confused if you try to change Mode later.
  vtkSetMacro(Mode,int);
  vtkGetMacro(Mode,int);
  //BTX
  enum
    {
    MODE_STREAM=1,
    MODE_TOPOLOGY=2,
    MODE_POINCARE=3,
    MODE_DISPLACEMENT=4
    };
  //ETX

  // Description:
  // If set then only forward traces is carried out.
  vtkSetMacro(ForwardOnly,int);
  vtkGetMacro(ForwardOnly,int);

  // Description:
  // Set integrator type. RK2=1, RK4=2, RK45=3
  void SetIntegratorType(int type);
  int GetIntegratorType(){ return this->IntegratorType; }
  //BTX
  enum
    {
    INTEGRATOR_NONE=0,
    INTEGRATOR_RK2=1,
    INTEGRATOR_RK4=2,
    INTEGRATOR_RK45=3
    };
  //ETX

  // Description:
  // Specify a uniform integration step unit for MinimumIntegrationStep,
  // InitialIntegrationStep, and MaximumIntegrationStep. NOTE: The valid
  // units are LENGTH_UNIT (1) and CELL_LENGTH_UNIT (2).
  void SetStepUnit(int unit);
  vtkGetMacro(StepUnit,int);

  // Description:
  // Specify the Minimum step size used for line integration.
  vtkSetMacro(MinStep,double);
  vtkGetMacro(MinStep,double);

  // Description:
  // Specify the Maximum step size used for line integration.
  vtkSetMacro(MaxStep,double);
  vtkGetMacro(MaxStep,double);

  // Description
  // Specify the maximum error tolerated throughout streamline integration.
  vtkSetMacro(MaxError,double);
  vtkGetMacro(MaxError,double);

  // Description
  // Specify the maximum number of steps for integrating a streamline.
  vtkSetMacro(MaxNumberOfSteps,vtkIdType);
  vtkGetMacro(MaxNumberOfSteps,vtkIdType);

  // Description:
  // Specify the maximum length of a streamline expressed in LENGTH_UNIT.
  vtkSetMacro(MaxLineLength,double);
  vtkGetMacro(MaxLineLength,double);

  // Description:
  // Specify the maximum time interval to integrate over.
  vtkSetMacro(MaxIntegrationInterval,double);
  vtkGetMacro(MaxIntegrationInterval,double);

  // Description
  // Specify the terminal speed value, below which integration is terminated.
  vtkSetMacro(NullThreshold,double);
  vtkGetMacro(NullThreshold,double);

  // Description:
  // Specify the minimum segment length
  vtkSetMacro(MinSegmentLength,double);
  vtkGetMacro(MinSegmentLength,double);

  // Description:
  // If set then comm world is used during reads. This will result in better
  // in-core preformance when there is enough memory for each process to
  // have it's own copy of the data. Note: use of comm world precludes
  // out-of-core operation.
  vtkSetMacro(UseCommWorld,int);
  vtkGetMacro(UseCommWorld,int);

  // Descrition:
  // If set then segments of field lines that are the result of a
  // periodic boundary condition are removed from the output.
  vtkSetMacro(CullPeriodicTransitions,int);
  vtkGetMacro(CullPeriodicTransitions,int);

  // Description:
  // If on then color map produced will only contain used colors.
  // NOTE: requires a global communication,
  vtkSetMacro(SqueezeColorMap,int);
  vtkGetMacro(SqueezeColorMap,int);

  // Description:
  // Sets the work unit (in number of seed points) for slave processes.
  vtkSetClampMacro(WorkerBlockSize,int,1,VTK_INT_MAX);
  vtkGetMacro(WorkerBlockSize,int);

  // Description:
  // Sets the work unit (in number of seed points) for the master. This should
  // be much less than the slave block size, so that the master can respond
  // timely to slave requests.
  vtkSetClampMacro(MasterBlockSize,int,0,VTK_INT_MAX);
  vtkGetMacro(MasterBlockSize,int);

  // Description:
  // Enable/disable the dynamic scheduler. The dynamic scheduler requires that
  // a seed generator be inserted into the pipeline, or that all seed points
  // are generated on all ranks.
  vtkSetMacro(UseDynamicScheduler,int);
  vtkGetMacro(UseDynamicScheduler,int);

  // Description:
  // Set the log level.
  // 0 -- no logging
  // 1 -- basic logging
  // .
  // n -- advanced logging
  vtkSetMacro(LogLevel,int);
  vtkGetMacro(LogLevel,int);

protected:
  vtkSQFieldTracer();
  ~vtkSQFieldTracer();

  // VTK Pipeline
  int FillInputPortInformation(int port,vtkInformation *info);
  int FillOutputPortInformation(int port,vtkInformation *info);
  int RequestData(vtkInformation *req, vtkInformationVector **input, vtkInformationVector *output);
  int RequestInformation(vtkInformation* req, vtkInformationVector** input, vtkInformationVector* output);
  int RequestUpdateExtent(vtkInformation* req, vtkInformationVector** input, vtkInformationVector* output);
  int RequestDataObject(vtkInformation *info,vtkInformationVector** input,vtkInformationVector* output);

private:
  //BTX
  // Description:
  // Integrate over all local cells. This assumes that each process has a unique
  // subset of the work (i.e. seed source cells are statically distributed),
  int IntegrateStatic(
      vtkIdType nCells,
      const char *fieldName,
      vtkSQOOCReader *oocr,
      vtkDataSet *&oocrCache,
      FieldTraceData *topoMap);

  // Description:
  // Distribute the work load according to a master-slave self scheduling scheme. All
  // seed cells must be present on all process, work is dished out by process 0 in
  // contiguous blocks of cell ids.
  int IntegrateDynamic(
      int procId,
      int nProcs,
      vtkIdType nCells,
      const char *fieldName,
      vtkSQOOCReader *oocr,
      vtkDataSet *&oocrCache,
      FieldTraceData *topoMap);

  // Description:
  // Integrate field lines seeded from a block of consecutive cell ids.
  int IntegrateBlock(
        IdBlock *sourceIds,
        FieldTraceData *topoMap,
        const char *fieldName,
        vtkSQOOCReader *oocr,
        vtkDataSet *&oocrCache);


  // Description:
  // Trace one field line from the given seed point, using the given out-of-core
  // reader. As segments are generated they are tested using the stermination
  // condition and terminated imediately. The last neighborhood read is stored
  // in the nhood parameter. It is up to the caller to delete this.
  void IntegrateOne(
        vtkSQOOCReader *oocR,
        vtkDataSet *&oocRCache,
        const char *fieldName,
        FieldLine *line,
        TerminationCondition *tcon);
  //ETX

  // Description:
  // Determine the start id of the cells in data relative
  // to the cells on all other processes in COMM_WORLD.
  // Requires a global communcation.
  unsigned long GetGlobalCellId(vtkDataSet *data);

  // Description:
  // Convert from cell fractional unit into length.
  void ClipStep(
      double& step,
      int stepSign,
      double& minStep,
      double& maxStep,
      double cellLength,
      double lineLength);

  // Description:
  // Convert from cell fractional unit into length.
  static double ConvertToLength(double interval,int unit,double cellLength);

  vtkSQFieldTracer(const vtkSQFieldTracer&);  // Not implemented.
  void operator=(const vtkSQFieldTracer&);  // Not implemented.

private:
  int WorldSize;
  int WorldRank;

  // Parameter controlling load balance
  int UseDynamicScheduler;
  int WorkerBlockSize;
  int MasterBlockSize;

  // Parameters controlling integration
  int ForwardOnly;
  int StepUnit;
  double MinStep;
  double MaxStep;
  double MaxError;
  vtkIdType MaxNumberOfSteps;
  double MaxLineLength;
  double MaxIntegrationInterval;
  double NullThreshold;
  int IntegratorType;
  vtkInitialValueProblemSolver* Integrator;
  double MinSegmentLength;
  static const double EPSILON;

  // Reader related
  int UseCommWorld;
  TerminationCondition *TermCon;

  // Output controls
  int Mode;
  int CullPeriodicTransitions;
  int SqueezeColorMap;

  int LogLevel;

  //BTX
  // units
  enum
    {
    ARC_LENGTH=1,
    CELL_FRACTION=2
    };
 //ETX
};

#endif
