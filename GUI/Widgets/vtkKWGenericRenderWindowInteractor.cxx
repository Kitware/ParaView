/*=========================================================================

Copyright (c) 1998-2003 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.

All rights reserved. No part of this software may be reproduced, distributed,
or modified, in any form or by any means, without permission in writing from
Kitware Inc.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.

=========================================================================*/
#include "vtkKWGenericRenderWindowInteractor.h"

#include "vtkKWRenderWidget.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindow.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkKWGenericRenderWindowInteractor);
vtkCxxRevisionMacro(vtkKWGenericRenderWindowInteractor, "1.7");

//----------------------------------------------------------------------------
vtkKWGenericRenderWindowInteractor::vtkKWGenericRenderWindowInteractor()
{
  this->RenderWidget = NULL;
}

//----------------------------------------------------------------------------
vtkKWGenericRenderWindowInteractor::~vtkKWGenericRenderWindowInteractor()
{
  this->SetRenderWidget(NULL);
}

//----------------------------------------------------------------------------
void vtkKWGenericRenderWindowInteractor::SetRenderWidget(
  vtkKWRenderWidget *widget)
{
  if (this->RenderWidget != widget)
    {
    // to avoid circular references
    this->RenderWidget = widget;
    if (this->RenderWidget != NULL)
      {
      this->SetRenderWindow(this->RenderWidget->GetRenderWindow());
      }
    else
      {
      this->SetRenderWindow(NULL);
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWGenericRenderWindowInteractor::Render()
{
  if (this->RenderWidget)
    {
    this->RenderWidget->Render();
    }
}

//----------------------------------------------------------------------------
void vtkKWGenericRenderWindowInteractor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "LastEventPosition: (" << this->LastEventPosition[0] << ", "
     << this->LastEventPosition[1] << ")" << endl;
  os << indent << "RenderWidget: " << this->RenderWidget << endl;
}
