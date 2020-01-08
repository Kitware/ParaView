/*=========================================================================

  Program:   ParaView
  Module:    vtkPVTransposeTable.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPVTransposeTable
 * @brief   create a subtable made with selected
 * columns of the input vtkTable and transpose it.
 *
 *
 * This ParaView filter allows to select the columns of the input table
 * that must be included in the transposed table. This filter can also
 * be use to extract a non transposed table made by the selected columns.
*/

#ifndef vtkPVTransposeTable_h
#define vtkPVTransposeTable_h

#include "vtkPVVTKExtensionsFiltersGeneralModule.h" //needed for exports
#include "vtkTransposeTable.h"

class PVTransposeTableInternal;

class VTKPVVTKEXTENSIONSFILTERSGENERAL_EXPORT vtkPVTransposeTable : public vtkTransposeTable
{
public:
  static vtkPVTransposeTable* New();
  vtkTypeMacro(vtkPVTransposeTable, vtkTransposeTable);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Interface for preparing selection of arrays in ParaView.
   */
  void EnableAttributeArray(const char*);
  void ClearAttributeArrays();
  //@}

  //@{
  /**
   * Can be used to bypass the transposition code. The output
   * is then the table made with the selected columns.
   */
  vtkGetMacro(DoNotTranspose, bool);
  vtkSetMacro(DoNotTranspose, bool);
  vtkBooleanMacro(DoNotTranspose, bool);
  //@}

protected:
  vtkPVTransposeTable();
  ~vtkPVTransposeTable() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  PVTransposeTableInternal* Internal;

  bool DoNotTranspose;

private:
  vtkPVTransposeTable(const vtkPVTransposeTable&) = delete;
  void operator=(const vtkPVTransposeTable&) = delete;
};

#endif // vtkPVTransposeTable_h
