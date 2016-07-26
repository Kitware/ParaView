/*
 * Copyright 2012 SciberQuest Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 *  * Neither name of SciberQuest Inc. nor the names of any contributors may be
 *    used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef vtkSQOOCReader_h
#define vtkSQOOCReader_h

#include "vtkSciberQuestModule.h" // for export macro
#include "vtkObject.h"

#ifdef SQTK_WITHOUT_MPI
typedef void * MPI_Comm;
#else
#include "SQMPICHWarningSupression.h" // for suppressing MPI warnings
#include "mpi.h" // for MPI_Comm
#endif

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
class VTKSCIBERQUEST_EXPORT vtkSQOOCReader : public vtkObject
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
  vtkTypeMacro(vtkSQOOCReader, vtkObject);
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
  vtkSQOOCReader(const vtkSQOOCReader&) VTK_DELETE_FUNCTION;
  void operator=(const vtkSQOOCReader&) VTK_DELETE_FUNCTION;

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
