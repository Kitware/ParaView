/*=========================================================================

  Program:   ParaView
  Module:    vtkSMContextNamedOptionsProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMContextNamedOptionsProxy
// .SECTION Description
//

#ifndef __vtkSMContextNamedOptionsProxy_h
#define __vtkSMContextNamedOptionsProxy_h

#include "vtkSMProxy.h"

class vtkChart;
class vtkTable;

class VTK_EXPORT vtkSMContextNamedOptionsProxy : public vtkSMProxy
{
public:
  static vtkSMContextNamedOptionsProxy* New();
  vtkTypeRevisionMacro(vtkSMContextNamedOptionsProxy, vtkSMProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get/Set series visibility for the series with the given name.
  void SetVisibility(const char* name, int visible);
  void SetLineThickness(const char* name, int value);
  void SetLineStyle(const char* name, int value);
  void SetColor(const char* name, double r, double g, double b);
  void SetAxisCorner(const char* name, int corner);
  void SetMarkerStyle(const char* name, int style);
  void SetLabel(const char* name, const char* label);

  // Description:
  // Set the X series to be used for the plots, if NULL then the index of the
  // y series should be used.
  void SetXSeriesName(const char* name);

//BTX
  // Description:
  // Sets the internal chart object whose options will be manipulated.
  void SetChart(vtkChart* chart);

  // Description:
  // Sets the internal table object that can be plotted.
  void SetTable(vtkTable* table);
//ETX

//BTX
protected:
  vtkSMContextNamedOptionsProxy();
  ~vtkSMContextNamedOptionsProxy();

  // Description:
  // Initializes the plots map, and adds a default series to plot
  void InitializePlotMap();

  // Description:
  // Called to update the property information on the property. It is assured
  // that the property passed in as an argument is a self property. Both the
  // overloads of UpdatePropertyInformation() call this method, so subclass can
  // override this method to perform special tasks.
  virtual void UpdatePropertyInformationInternal(vtkSMProperty*);

private:
  vtkSMContextNamedOptionsProxy(const vtkSMContextNamedOptionsProxy&); // Not implemented
  void operator=(const vtkSMContextNamedOptionsProxy&); // Not implemented

  class vtkInternals;
  vtkInternals* Internals;
//ETX
};

#endif
