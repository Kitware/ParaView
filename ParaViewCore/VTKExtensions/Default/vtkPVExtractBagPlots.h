/*=========================================================================

  Program:   ParaView
  Module:    vtkPVExtractBagPlots.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVExtractBagPlots
//
// .SECTION Description

#ifndef vtkPVExtractBagPlots_h
#define vtkPVExtractBagPlots_h

#include "vtkPVVTKExtensionsDefaultModule.h" //needed for exports
#include "vtkMultiBlockDataSetAlgorithm.h"

class PVExtractBagPlotsInternal;

class VTKPVVTKEXTENSIONSDEFAULT_EXPORT vtkPVExtractBagPlots
  : public vtkMultiBlockDataSetAlgorithm
{
public:
  static vtkPVExtractBagPlots* New();
  vtkTypeMacro(vtkPVExtractBagPlots, vtkMultiBlockDataSetAlgorithm);
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Interface for preparing selection of arrays in ParaView.
  void EnableAttributeArray(const char*);
  void ClearAttributeArrays();

  // Description:
  // Set/get if the process must be done on the transposed of the input table
  vtkGetMacro(TransposeTable, bool);
  vtkSetMacro(TransposeTable, bool);
  vtkBooleanMacro(TransposeTable, bool);

  // Description:
  // Set/get if the PCA must be done in robust mode
  vtkGetMacro(RobustPCA, bool);
  vtkSetMacro(RobustPCA, bool);

  // Description:
  // Set/get the sigma parameter for the HDR filter
  vtkGetMacro(Sigma, double);
  vtkSetMacro(Sigma, double);

  // Description:
  // Set/get the size of the grid to compute the PCA on.
  // 100 by default.
  vtkGetMacro(GridSize, int);
  vtkSetMacro(GridSize, int);

protected:
  vtkPVExtractBagPlots();
  virtual ~vtkPVExtractBagPlots();

  virtual int FillInputPortInformation( int port, vtkInformation *info );

  int RequestData(vtkInformation*,
    vtkInformationVector**,
    vtkInformationVector*);

  PVExtractBagPlotsInternal* Internal;

  bool TransposeTable;
  bool RobustPCA;
  double Sigma;
  int GridSize;

private:
  vtkPVExtractBagPlots( const vtkPVExtractBagPlots& ); // Not implemented.
  void operator = ( const vtkPVExtractBagPlots& ); // Not implemented.
};

#endif // vtkPVExtractBagPlots_h
