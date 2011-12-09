#ifndef __vtkNektarReader_h
#define __vtkNektarReader_h

#include <iostream>
#include <fstream>
#include <vector>
//#include <algorithm>

#include <nektar.h>
#include <gen_utils.h>

#include <mpi.h>

#include "vtkUnstructuredGridAlgorithm.h"
#include "vtkPoints.h"
#include "vtkCellType.h"


class vtkDataArraySelection;

#include "nektarObject.h"


class VTK_EXPORT vtkNektarReader : public vtkUnstructuredGridAlgorithm
{
public:
  static vtkNektarReader *New();
  vtkTypeMacro(vtkNektarReader,vtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  unsigned long GetMTime();

  // Description:
  // Get the output data object for a port on this algorithm.
  vtkUnstructuredGrid* GetOutput();
  vtkUnstructuredGrid* GetOutput(int);
  virtual void SetOutput(vtkDataObject* d);

  // Description:
  // see vtkAlgorithm for details
  virtual int ProcessRequest(vtkInformation*,
                             vtkInformationVector**,
                             vtkInformationVector*);

  // this method is not recommended for use, but lots of old style filters
  // use it
  vtkDataObject *GetInput(int port);
  vtkDataObject *GetInput() { return this->GetInput(0); };
  vtkUnstructuredGrid *GetUnstructuredGridInput(int port);

  // Description:
  // Set an input of this algorithm. You should not override these
  // methods because they are not the only way to connect a pipeline.
  // Note that these methods support old-style pipeline connections.
  // When writing new code you should use the more general
  // vtkAlgorithm::SetInputConnection().  These methods transform the
  // input index to the input port index, not an index of a connection
  // within a single port.
  void SetInput(vtkDataObject *);
  void SetInput(int, vtkDataObject*);

  // Description:
  // Add an input of this algorithm.  Note that these methods support
  // old-style pipeline connections.  When writing new code you should
  // use the more general vtkAlgorithm::AddInputConnection().  See
  // SetInput() for details.
  void AddInput(vtkDataObject *);
  void AddInput(int, vtkDataObject*);

  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

  vtkSetStringMacro(DataFileName);
  vtkGetStringMacro(DataFileName);

  // Description:
  // Which TimeStep to read.
  vtkSetMacro(TimeStep, int);
  vtkGetMacro(TimeStep, int);

  vtkGetMacro(NumberOfTimeSteps, int);
  // Description:
  // Returns the available range of valid integer time steps.
  vtkGetVector2Macro(TimeStepRange,int);
  vtkSetVector2Macro(TimeStepRange,int);


  // set/get the resolution to use for each element of the input grid
  void SetElementResolution(int);
  int GetElementResolution();


    // Description:
  // Get the number of point arrays available in the input.
  int GetNumberOfPointArrays(void);

  // Description:
  // Get the name of the  point array with the given index in
  // the input.
  const char* GetPointArrayName(int index);

  // Description:
  // Get/Set whether the point array with the given name is to
  // be read.
  int GetPointArrayStatus(const char* name);
  void SetPointArrayStatus(const char* name, int status);

  // Description:
  // Turn on/off all point arrays.
  void DisableAllPointArrays();
  void EnableAllPointArrays();


  // Description:
  // Get the number of derived variables available in the input.
  int GetNumberOfDerivedVariableArrays(void);

  // Description:
  // Get the name of the  derived variable array with the given index in
  // the input.
  const char* GetDerivedVariableArrayName(int index);

  // Description:
  // Get/Set whether the derived variable array with the given name is to
  // be read.
  int GetDerivedVariableArrayStatus(const char* name);
  void SetDerivedVariableArrayStatus(const char* name, int status);

  // Description:
  // Turn on/off all derived variable arrays.
  void DisableAllDerivedVariableArrays();
  void EnableAllDerivedVariableArrays();



protected:
  vtkNektarReader();
  ~vtkNektarReader();

  char* FileName;
  char* DataFileName;
  int ElementResolution;
  int nfields;
  int my_patch_id;
  int my_rank;

  nektarList *myList;
  nektarObject *curObj;
  int displayed_step;
  int memory_step;
  int requested_step;

  FileList       fl;
  Element_List **master;

  static int next_patch_id;

  // to be read from .nektar file
  char paramFile[256];
  char reaFile[256];
  char rstFile[256];

  char  p_rea_file[256];
  char  p_rst_dir[256];
  char  p_rst_base[256];
  char  p_rst_ext[256];
  char  p_rst_file[256];
  char  p_rst_format[256];
  int   p_rst_start;
  int   p_rst_inc;
  int   p_rst_num;
  int   p_rst_digits;
  float p_time_start;
  float p_time_inc;

  void setActive();  // set my_patch_id as the active one
  static int getNextPatchID(){return(next_patch_id++);}


  vtkDataArraySelection* PointDataArraySelection;
  vtkDataArraySelection* DerivedVariableDataArraySelection;

  // copy the data from nektar to pv
  void updateVtuData(vtkUnstructuredGrid* pv_ugrid);

  vtkPoints* points;
  vtkUnstructuredGrid* ugrid;
  bool CALC_GEOM_FLAG;  // true = need to calculate geometry; false = geom is up to date
  bool READ_GEOM_FLAG;  // true = need geom from disk
  bool IAM_INITIALLIZED;
  bool I_HAVE_DATA;
  bool FIRST_DATA;

  int TimeStep;
  int ActualTimeStep;
  int NumberOfTimeSteps;
  double TimeValue;
  int TimeStepRange[2];
  double *TimeSteps;

  // Time query function. Called by ExecuteInformation().
  // Fills the TimestepValues array.
  void GetAllTimes(vtkInformationVector*);

  // Description:
  // Populates the TIME_STEPS and TIME_RANGE keys based on file metadata.
  void AdvertiseTimeSteps( vtkInformation* outputInfo );


  // convenience method
  virtual int RequestInformation(vtkInformation* request,
                                 vtkInformationVector** inputVector,
                                 vtkInformationVector* outputVector);

  // Description:
  // This is called by the superclass.
  // This is the method you should override.
  virtual int RequestData(vtkInformation* request,
                          vtkInformationVector** inputVector,
                          vtkInformationVector* outputVector);

  // Description:
  // This is called by the superclass.
  // This is the method you should override.
  virtual int RequestUpdateExtent(vtkInformation*,
                                  vtkInformationVector**,
                                  vtkInformationVector*);

  // Description:
  // This method is the old style execute method
  virtual void ExecuteData(vtkDataObject *output);
  virtual void Execute();

  // see algorithm for more info
  virtual int FillOutputPortInformation(int port, vtkInformation* info);
  virtual int FillInputPortInformation(int port, vtkInformation* info);

private:
  vtkNektarReader(const vtkNektarReader&);  // Not implemented.
  void operator=(const vtkNektarReader&);  // Not implemented.
};

#endif
