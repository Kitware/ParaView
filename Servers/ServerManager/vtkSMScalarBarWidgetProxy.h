/*=========================================================================

  Program:   ParaView
  Module:    vtkSMScalarBarWidgetProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMScalarBarWidgetProxy - 
// .SECTION Description
// vtkSMScalarBarWidgetProxy is the proxy for vtkScalarBarWidget.

#ifndef __vtkSMScalarBarWidgetProxy_h
#define __vtkSMScalarBarWidgetProxy_h

#include "vtkSMInteractorObserverProxy.h"

class VTK_EXPORT vtkSMScalarBarWidgetProxy : public vtkSMInteractorObserverProxy
{
public:
  static vtkSMScalarBarWidgetProxy* New();
  vtkTypeRevisionMacro(vtkSMScalarBarWidgetProxy, vtkSMInteractorObserverProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the scalr bar title string.
  virtual void SetTitle(const char* title);


  // Description:
  // Enable/Disable the scalar bar. Overridden to set the current renderer on the
  // ScalarBarWidget.
  virtual void SetEnabled(int enable);

  // Description:
  // Set the format for the labels on the ScalarBar
  virtual void SetLabelFormat(const char* format);

  // Description:
  // GetSet the position and orientation for the ScalarBar.
  vtkGetVector2Macro(Position1,double);
  vtkSetVector2Macro(Position1,double);
  vtkGetVector2Macro(Position2,double);
  vtkSetVector2Macro(Position2,double);
  vtkGetMacro(Orientation,int);
  vtkSetMacro(Orientation,int);
 
  // Description:
  // Push the properties on to the vtk objects.
  virtual void UpdateVTKObjects();

  // Description:
  // Set the visibility of the scalar bar
  virtual void SetVisibility(int visible)
    { this->SetEnabled(visible); }

  // Description:
  // Set the text property for title
  virtual void SetTitleFormatColor(double color[3]);
  virtual void SetTitleFormatOpacity(double opacity);
  virtual void SetTitleFormatFont(int font);
  virtual void SetTitleFormatBold(int bold);
  virtual void SetTitleFormatItalic(int italic);
  virtual void SetTitleFormatShadow(int shadow);
  
  // Description:
  // Set the text property for the labels
  virtual void SetLabelFormatColor(double color[3]);
  virtual void SetLabelFormatOpacity(double opacity);
  virtual void SetLabelFormatFont(int font);
  virtual void SetLabelFormatBold(int bold);
  virtual void SetLabelFormatItalic(int italic);
  virtual void SetLabelFormatShadow(int shadow);

  virtual void SaveInBatchScript(ofstream* file);

  // Description:
  // Get/Set Lookuptable proxy for the scalar bar.
  void SetLookupTable(vtkSMProxy* lut);
  //vtkSMProxy* GetLookupTable();

  
  // Description:
  // Called when the display is added/removed to/from a RenderModule.
  virtual void AddToRenderModule(vtkSMRenderModuleProxy*);
  virtual void RemoveFromRenderModule(vtkSMRenderModuleProxy*);
protected:
//BTX
  vtkSMScalarBarWidgetProxy();
  ~vtkSMScalarBarWidgetProxy();
  friend class vtkPVColorMap;

  virtual void CreateVTKObjects(int numObjects);
  
  void ExecuteEvent(vtkObject*obj, unsigned long event, void*p);
  
  double Position1[2];
  double Position2[2];
  int Orientation;

private:
  vtkSMScalarBarWidgetProxy(const vtkSMScalarBarWidgetProxy&); // Not implemented
  void operator=(const vtkSMScalarBarWidgetProxy&); // Not implemented
//ETX
};

#endif

