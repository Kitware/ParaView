/*=========================================================================

  Program:   ParaView
  Module:    vtkPVNullSource.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPVNullSource
 * @brief   source for NULL data.
 *
 * This is a source for null data. Although this actually produces a
 * vtkPolyLine paraview blocks all data information from this source resulting
 * in it being treated as a null source.
*/

#ifndef vtkPVNullSource_h
#define vtkPVNullSource_h

#include "vtkLineSource.h"
#include "vtkPVVTKExtensionsCoreModule.h" //needed for exports

class VTKPVVTKEXTENSIONSCORE_EXPORT vtkPVNullSource : public vtkLineSource
{
public:
  static vtkPVNullSource* New();
  vtkTypeMacro(vtkPVNullSource, vtkLineSource);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkPVNullSource();
  ~vtkPVNullSource() override;

private:
  vtkPVNullSource(const vtkPVNullSource&) = delete;
  void operator=(const vtkPVNullSource&) = delete;
};

#endif
