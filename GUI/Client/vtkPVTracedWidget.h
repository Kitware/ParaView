/*=========================================================================

  Module:    vtkPVTracedWidget.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVTracedWidget - a vtkKWWidget with trace capabilities
// .SECTION Description
// This class is a subclass of vtkKWWidget with trace methods..
// .SECTION See Also
// vtkPVTraceHelper

#ifndef __vtkPVTracedWidget_h
#define __vtkPVTracedWidget_h

#include "vtkKWWidget.h"

class vtkPVTraceHelper;

class VTK_EXPORT vtkPVTracedWidget : public vtkKWWidget
{
public:
  static vtkPVTracedWidget* New();
  vtkTypeRevisionMacro(vtkPVTracedWidget,vtkKWWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the trace helper framework.
  // IMPORTANT: the vtkPVTraceHelper object is lazy-allocated, i.e.
  // allocated only when it is needed, as GetTraceHelper() is called.
  // Therefore, to check if the instance *has* a trace helper, use 
  // HasTraceHelper(), not GetTraceHelper().
  virtual int HasTraceHelper();
  virtual vtkPVTraceHelper* GetTraceHelper();

protected:
  vtkPVTracedWidget();
  ~vtkPVTracedWidget();

private:
  
  // In private: to allow lazy evaluation.

  vtkPVTraceHelper* TraceHelper;

  vtkPVTracedWidget(const vtkPVTracedWidget&); // Not implemented
  void operator=(const vtkPVTracedWidget&); // Not implemented
};

#endif
