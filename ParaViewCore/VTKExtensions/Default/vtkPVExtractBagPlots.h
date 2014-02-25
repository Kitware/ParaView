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

#ifndef __vtkPVExtractBagPlots_h
#define __vtkPVExtractBagPlots_h

#include "vtkPVVTKExtensionsDefaultModule.h" //needed for exports
#include "vtkTransposeTable.h"

class PVExtractBagPlotsInternal;

class VTKPVVTKEXTENSIONSDEFAULT_EXPORT vtkPVExtractBagPlots : public vtkTransposeTable
{
public:
  static vtkPVExtractBagPlots* New();
  vtkTypeMacro(vtkPVExtractBagPlots, vtkTransposeTable);
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

protected:
  vtkPVExtractBagPlots();
  virtual ~vtkPVExtractBagPlots();

  int RequestData(vtkInformation*,
    vtkInformationVector**,
    vtkInformationVector*);

  PVExtractBagPlotsInternal* Internal;

  bool TransposeTable;
  bool RobustPCA;
  double Sigma;

private:
  vtkPVExtractBagPlots( const vtkPVExtractBagPlots& ); // Not implemented.
  void operator = ( const vtkPVExtractBagPlots& ); // Not implemented.
};

#endif // __vtkPVExtractBagPlots_h
