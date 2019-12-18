/*=========================================================================

  Program:   ParaView
  Module:    vtkPVConnectivityFilter.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPVConnectivityFilter
 * @brief   change the defaults for vtkConnectivityFilter
 *
 * vtkPVConnectivityFilter is a subclass of vtkConnectivityFilter.  It
 * changes the default settings.  We want different defaults than
 * vtkConnectivityFilter has, but we don't want the user to have access to
 * these parameters in the UI.
*/

#ifndef vtkPVConnectivityFilter_h
#define vtkPVConnectivityFilter_h

#include "vtkConnectivityFilter.h"
#include "vtkPVVTKExtensionsFiltersGeneralModule.h" //needed for exports

class VTKPVVTKEXTENSIONSFILTERSGENERAL_EXPORT vtkPVConnectivityFilter : public vtkConnectivityFilter
{
public:
  vtkTypeMacro(vtkPVConnectivityFilter, vtkConnectivityFilter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static vtkPVConnectivityFilter* New();

protected:
  vtkPVConnectivityFilter();
  ~vtkPVConnectivityFilter() override{};

private:
  vtkPVConnectivityFilter(const vtkPVConnectivityFilter&) = delete;
  void operator=(const vtkPVConnectivityFilter&) = delete;
};

#endif
