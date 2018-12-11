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
/**
 * @class   vtkPVExtractVOI
 * @brief    select piece (e.g., volume of interest) and/or subsample structured dataset
 *
 * vtkPVExtractVOI is a wrapper around vtkExtractVOI, vtkExtractGrid and
 * vtkExtractRectilinearGrid. It choose the right filter depending on
 * input and passes the necessary parameters.
 *
 * @sa
 * vtkExtractVOI vtkExtractGrid vtkExtractRectilinearGrid
*/

#ifndef vtkPVExtractVOI_h
#define vtkPVExtractVOI_h

#include "vtkDataSetAlgorithm.h"
#include "vtkPVVTKExtensionsDefaultModule.h" //needed for exports

class vtkDataObject;
class vtkDataSet;
class vtkExtractGrid;
class vtkExtractRectilinearGrid;
class vtkExtractVOI;

class VTKPVVTKEXTENSIONSDEFAULT_EXPORT vtkPVExtractVOI : public vtkDataSetAlgorithm
{

public:
  static vtkPVExtractVOI* New();
  vtkTypeMacro(vtkPVExtractVOI, vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Specify i-j-k (min,max) pairs to extract. The resulting structured grid
   * dataset can be of any topological dimension (i.e., point, line, plane,
   * or 3D grid).
   */
  vtkSetVector6Macro(VOI, int);
  vtkGetVectorMacro(VOI, int, 6);
  //@}

  //@{
  /**
   * Set the sampling rate in the i, j, and k directions. If the rate is > 1,
   * then the resulting VOI will be subsampled representation of the input.
   * For example, if the SampleRate=(2,2,2), every other point will be
   * selected, resulting in a volume 1/8th the original size.
   */
  vtkSetVector3Macro(SampleRate, int);
  vtkGetVectorMacro(SampleRate, int, 3);
  //@}

  //@{
  /**
   * Set/get the individual components of the sample rate.
   */
  void SetSampleRateI(int ratei);
  void SetSampleRateJ(int ratej);
  void SetSampleRateK(int ratek);
  int GetSampleRateI() { return this->SampleRate[0]; }
  int GetSampleRateJ() { return this->SampleRate[1]; }
  int GetSampleRateK() { return this->SampleRate[2]; }
  //@}

  //@{
  /**
   * Control whether to enforce that the "boundary" of the grid is output in
   * the subsampling process. (This ivar only has effect when the SampleRate
   * in any direction is not equal to 1.) When this ivar IncludeBoundary is
   * on, the subsampling will always include the boundary of the grid even
   * though the sample rate is not an even multiple of the grid
   * dimensions. (By default IncludeBoundary is off.)
   */
  vtkSetMacro(IncludeBoundary, int);
  vtkGetMacro(IncludeBoundary, int);
  vtkBooleanMacro(IncludeBoundary, int);
  //@}

protected:
  vtkPVExtractVOI();
  ~vtkPVExtractVOI() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestUpdateExtent(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  int VOI[6];
  int SampleRate[3];
  int IncludeBoundary;

  vtkExtractVOI* ExtractVOI;
  vtkExtractGrid* ExtractGrid;
  vtkExtractRectilinearGrid* ExtractRG;

  void ReportReferences(vtkGarbageCollector*) override;

private:
  vtkPVExtractVOI(const vtkPVExtractVOI&) = delete;
  void operator=(const vtkPVExtractVOI&) = delete;
};

#endif
