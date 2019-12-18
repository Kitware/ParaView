/*=========================================================================

  Program:   ParaView
  Module:    vtkPVRepresentedDataInformation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPVRepresentedDataInformation
 *
 * vtkPVRepresentedDataInformation is a vtkPVDataInformation subclass that knows
 * how to gather rendered data-information from a vtkPVDataRepresentation.
*/

#ifndef vtkPVRepresentedDataInformation_h
#define vtkPVRepresentedDataInformation_h

#include "vtkPVDataInformation.h"
#include "vtkRemotingViewsModule.h" //needed for exports

class VTKREMOTINGVIEWS_EXPORT vtkPVRepresentedDataInformation : public vtkPVDataInformation
{
public:
  static vtkPVRepresentedDataInformation* New();
  vtkTypeMacro(vtkPVRepresentedDataInformation, vtkPVDataInformation);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Transfer information about a single object into this object.
   */
  void CopyFromObject(vtkObject*) override;

protected:
  vtkPVRepresentedDataInformation();
  ~vtkPVRepresentedDataInformation() override;

private:
  vtkPVRepresentedDataInformation(const vtkPVRepresentedDataInformation&) = delete;
  void operator=(const vtkPVRepresentedDataInformation&) = delete;
};

#endif
