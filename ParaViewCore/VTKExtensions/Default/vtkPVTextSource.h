/*=========================================================================

  Program:   ParaView
  Module:    vtkPVTextSource.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPVTextSource
 * @brief   source that generates a 1x1 vtkTable with a single
 * string data.
 *
 * vtkPVTextSource is used to generate a table with a single string.
*/

#ifndef vtkPVTextSource_h
#define vtkPVTextSource_h

#include "vtkPVVTKExtensionsDefaultModule.h" //needed for exports
#include "vtkTableAlgorithm.h"

class VTKPVVTKEXTENSIONSDEFAULT_EXPORT vtkPVTextSource : public vtkTableAlgorithm
{
public:
  static vtkPVTextSource* New();
  vtkTypeMacro(vtkPVTextSource, vtkTableAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Get/Set the text string to generate in the output.
   */
  vtkSetStringMacro(Text);
  vtkGetStringMacro(Text);
  //@}

protected:
  vtkPVTextSource();
  ~vtkPVTextSource();

  virtual int FillInputPortInformation(int port, vtkInformation* info) VTK_OVERRIDE;
  virtual int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) VTK_OVERRIDE;
  virtual int RequestInformation(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) VTK_OVERRIDE;
  char* Text;

private:
  vtkPVTextSource(const vtkPVTextSource&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPVTextSource&) VTK_DELETE_FUNCTION;
};

#endif
