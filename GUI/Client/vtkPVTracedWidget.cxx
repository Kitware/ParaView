/*=========================================================================

  Module:    vtkPVTracedWidget.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkPVTracedWidget.h"

#include "vtkPVTraceHelper.h"
#include "vtkObjectFactory.h"

#include <kwsys/SystemTools.hxx>

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkPVTracedWidget );
vtkCxxRevisionMacro(vtkPVTracedWidget, "1.1");

int vtkPVTracedWidgetCommand(ClientData cd, Tcl_Interp *interp,
                             int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVTracedWidget::vtkPVTracedWidget()
{
  this->TraceHelper = NULL;
}

//----------------------------------------------------------------------------
vtkPVTracedWidget::~vtkPVTracedWidget()
{
  if (this->TraceHelper)
    {
    this->TraceHelper->Delete();
    this->TraceHelper = NULL;
    }
}

//----------------------------------------------------------------------------
int vtkPVTracedWidget::HasTraceHelper()
{
  return this->TraceHelper ? 1 : 0;
}

//----------------------------------------------------------------------------
vtkPVTraceHelper* vtkPVTracedWidget::GetTraceHelper()
{
  // Lazy allocation. Create the helper only when it is needed

  if (!this->TraceHelper)
    {
    this->TraceHelper = vtkPVTraceHelper::New();
    this->TraceHelper->SetObject(this);
    }

  return this->TraceHelper;
}

//----------------------------------------------------------------------------
void vtkPVTracedWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  os << indent << "TraceHelper: ";
  if (this->TraceHelper)
    {
    os << this->TraceHelper << endl;
    }
  else
    {
    os << "None" << endl;
    }
}

