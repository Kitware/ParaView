/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.
*/
#ifndef __vtkSQOOCReader_h
#define __vtkSQOOCReader_h

#include "vtkObject.h"
#include "mpi.h"

class vtkInformation;
class vtkInformationObjectBaseKey;
class vtkInformationDoubleVectorKey;
class vtkInformationIntegerVectorKey;
class vtkDataSet;
class CartesianBounds;

/// Interface class for Out-Of-Core (OOC) file access.
/**
Allow one to read in chuncks of data as needed. A specific
chunk of data is identified to be read by providing a point
in which the chunk should reside. The implementation may
implement caching as desired but this is not required.
This class also provides a number of keys that should be used
by meta readers.
*/
class VTK_EXPORT vtkSQOOCReader : public vtkObject
{
public:
  /// \section Pipeline \@{
  /**
  This key is used to pass the actual reader from the meta reader
  to downstream filters. The meta reader at the head of the pipeline
  initializes and sets the reader into the pipeline information
  then down strem filters may read data as needed. The object set 
  must implement the vtkSQOOCReader interface and be reader to read
  once set into the pipeline.
  */
  static vtkInformationObjectBaseKey *READER();

  /**
  This key is used to pass the dataset bounds downstream.
  */
  // TODO use vtk WHOLE_BOUNDING_BOX key instead.
  static vtkInformationDoubleVectorKey *BOUNDS();
  static vtkInformationIntegerVectorKey *PERIODIC_BC();

public:
  vtkTypeRevisionMacro(vtkSQOOCReader, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);


  // Description:
  // Set the communicator to open the file on. optional.
  virtual void SetCommunicator(MPI_Comm) {}

  /// \section IO \@{
  /**
  Open the dataset for reading. In the case ofg an error 0 is
  returned.
  */
  virtual int Open()=0;

  /**
  Close the dataset.
  */
  virtual void Close()=0;

  /**
  Return the block of data containing point, p. The block
  bounds are to be returned in the the WrokingDomain. These
  may differ from the bounds of the dataset returned. For
  example when providing ghost cells, that the filter should
  ignore.
  */
  virtual vtkDataSet *ReadNeighborhood(
      const double p[3],
      CartesianBounds &WorkingDomain)=0;

  /**
  Turn on an array to be read.
  */
  virtual void ActivateArray(const char *arrayName)=0;
  /**
  Turn off an array to be read.
  */
  virtual void DeActivateArray(const char *arrayName)=0;
  virtual void DeActivateAllArrays()=0;

  /**
  Set the time step index or time to read.
  */
  vtkSetMacro(Time,double);
  vtkGetMacro(Time,double);
  vtkSetMacro(TimeIndex,int);
  vtkGetMacro(TimeIndex,int);

  /**
  Set the number of ghost cells to use during a read. Default is
  0.
  */
  void SetNumberOfGhostCells(int nghost){ this->NGhostCells=nghost; }
  int GetNumberOfGhostCells(){ return this->NGhostCells; }
  /// \@}

private:
  vtkSQOOCReader(const vtkSQOOCReader &o);
  const vtkSQOOCReader &operator=(const vtkSQOOCReader &o);

protected:
  vtkSQOOCReader()
      :
    NGhostCells(0),
    TimeIndex(0),
    Time(0)
      {};
  virtual ~vtkSQOOCReader(){};

protected:
  int NGhostCells;
  int TimeIndex;
  double Time;
};

#endif
