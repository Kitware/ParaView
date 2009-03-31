/*=========================================================================

  Program:   ParaView
  Module:    vtkSMChartNamedOptionsModelProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMChartNamedOptionsModelProxy
// .SECTION Description
//

#ifndef __vtkSMChartNamedOptionsModelProxy_h
#define __vtkSMChartNamedOptionsModelProxy_h

#include "vtkSMProxy.h"

class vtkQtChartSeriesOptions;
class vtkQtChartTableRepresentation;
class vtkQtChartNamedSeriesOptionsModel;

class VTK_EXPORT vtkSMChartNamedOptionsModelProxy : public vtkSMProxy
{
public:
  vtkTypeRevisionMacro(vtkSMChartNamedOptionsModelProxy, vtkSMProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  vtkQtChartNamedSeriesOptionsModel* GetOptionsModel();

  // Description:
  // Get/Set series visibility for the series with the given name.
  void SetVisibility(const char* name, int visible);

  void SetLineColor(const char* name, double r, double g, double b);
  void SetLineThickness(const char* name, int value);
  void SetLineStyle(const char* name, int value);

//BTX
protected:
  vtkSMChartNamedOptionsModelProxy();
  ~vtkSMChartNamedOptionsModelProxy();

  friend class vtkSMChartTableRepresentationProxy;

  // Description:
  // Returns the options for a series with the name. A new instance is created
  // if none exists.
  vtkQtChartSeriesOptions* GetOptions(const char* name);

  // Description:
  // Creates internal objects that depend on the representation.
  void CreateObjects(vtkQtChartTableRepresentation* repr);

  // Description:
  // Subclasses must override this method to create the right type of options
  // for the type of layer/view/representation they are to be used for.
  virtual vtkQtChartSeriesOptions* NewOptions() = 0;

  // Description:
  // Called to update the property information on the property. It is assured
  // that the property passed in as an argument is a self property. Both the
  // overloads of UpdatePropertyInformation() call this method, so subclass can
  // override this method to perform special tasks.
  virtual void UpdatePropertyInformationInternal(vtkSMProperty*);

private:
  vtkSMChartNamedOptionsModelProxy(const vtkSMChartNamedOptionsModelProxy&); // Not implemented
  void operator=(const vtkSMChartNamedOptionsModelProxy&); // Not implemented

  class vtkInternals;
  vtkInternals* Internals;
//ETX
};

#endif

