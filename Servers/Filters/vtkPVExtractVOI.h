/*=========================================================================

  Program:   ParaView
  Module:    vtkPVExtractVOI.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVExtractVOI -  select piece (e.g., volume of interest) and/or subsample structured dataset
// .SECTION Description
// vtkPVExtractVOI is a wrapper around vtkExtractVOI, vtkExtractGrid and
// vtkExtractRectilinearGrid. It choose the right filter depending on
// input and passes the necessary parameters.

// .SECTION See Also
// vtkExtractVOI vtkExtractGrid vtkExtractRectilinearGrid

#ifndef __vtkPVExtractVOI_h
#define __vtkPVExtractVOI_h

#include "vtkDataSetAlgorithm.h"

class vtkDataObject;
class vtkDataSet;
class vtkExtractGrid;
class vtkExtractRectilinearGrid;
class vtkExtractVOI;

class VTK_EXPORT vtkPVExtractVOI : public vtkDataSetAlgorithm
{

public:
  static vtkPVExtractVOI *New();
  vtkTypeMacro(vtkPVExtractVOI,vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify i-j-k (min,max) pairs to extract. The resulting structured grid
  // dataset can be of any topological dimension (i.e., point, line, plane,
  // or 3D grid). 
  vtkSetVector6Macro(VOI,int);
  vtkGetVectorMacro(VOI,int,6);

  // Description:
  // Set the sampling rate in the i, j, and k directions. If the rate is > 1,
  // then the resulting VOI will be subsampled representation of the input.
  // For example, if the SampleRate=(2,2,2), every other point will be
  // selected, resulting in a volume 1/8th the original size.
  vtkSetVector3Macro(SampleRate, int);
  vtkGetVectorMacro(SampleRate, int, 3);

  // Description:
  // Set/get the individual components of the sample rate.
  void SetSampleRateI(int ratei);
  void SetSampleRateJ(int ratej);
  void SetSampleRateK(int ratek);
  int GetSampleRateI() { return this->SampleRate[0]; }
  int GetSampleRateJ() { return this->SampleRate[1]; }
  int GetSampleRateK() { return this->SampleRate[2]; }
  
  // Description:
  // Control whether to enforce that the "boundary" of the grid is output in
  // the subsampling process. (This ivar only has effect when the SampleRate
  // in any direction is not equal to 1.) When this ivar IncludeBoundary is
  // on, the subsampling will always include the boundary of the grid even
  // though the sample rate is not an even multiple of the grid
  // dimensions. (By default IncludeBoundary is off.)
  vtkSetMacro(IncludeBoundary,int);
  vtkGetMacro(IncludeBoundary,int);
  vtkBooleanMacro(IncludeBoundary,int);

protected:
  vtkPVExtractVOI();
  ~vtkPVExtractVOI();

  virtual int RequestData(
    vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  virtual int RequestInformation(
    vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  virtual int RequestUpdateExtent(
    vtkInformation *, vtkInformationVector **, vtkInformationVector *);

  int VOI[6];
  int SampleRate[3];
  int IncludeBoundary;

  vtkExtractVOI* ExtractVOI;
  vtkExtractGrid* ExtractGrid;
  vtkExtractRectilinearGrid* ExtractRG;

  virtual void ReportReferences(vtkGarbageCollector*);
private:
  vtkPVExtractVOI(const vtkPVExtractVOI&);  // Not implemented.
  void operator=(const vtkPVExtractVOI&);  // Not implemented.
};

#endif



