/*=========================================================================

  Program:   ParaView
  Module:    vtkPVQuadViewInformation.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVQuadViewInformation.h"

#include "vtkClientServerStream.h"
#include "vtkInformation.h"
#include "vtkMultiProcessStream.h"
#include "vtkObjectFactory.h"
#include "vtkPVQuadRenderView.h"

#include <vector>
#include <string>

vtkStandardNewMacro(vtkPVQuadViewInformation);

//----------------------------------------------------------------------------
vtkPVQuadViewInformation::vtkPVQuadViewInformation()
{
  this->RootOnly = 1;
  this->XLabel = this->YLabel = this->ZLabel = this->ScalarLabel = NULL;
  this->Initialize();
}

//----------------------------------------------------------------------------
vtkPVQuadViewInformation::~vtkPVQuadViewInformation()
{
  this->SetScalarLabel(NULL);
  this->SetXLabel(NULL);
  this->SetYLabel(NULL);
  this->SetZLabel(NULL);
}

//----------------------------------------------------------------------------
void vtkPVQuadViewInformation::CopyParametersToStream(vtkMultiProcessStream& str)
{
  str << 123654;
}

//----------------------------------------------------------------------------
void vtkPVQuadViewInformation::CopyParametersFromStream(vtkMultiProcessStream& str)
{
  int magic_number;
  str >> magic_number;
  if (magic_number != 123654)
    {
    vtkErrorMacro("Magic number mismatch.");
    }
}

//----------------------------------------------------------------------------
void vtkPVQuadViewInformation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "XLabel: " << (this->XLabel ? this->XLabel : "(NULL)")
     << " - Value: " << this->Values[0] << endl;
  os << indent << "YLabel: " << (this->YLabel ? this->YLabel : "(NULL)")
     << " - Value: " << this->Values[1] << endl;
  os << indent << "ZLabel: " << (this->ZLabel ? this->ZLabel : "(NULL)")
     << " - Value: " << this->Values[2] << endl;
  os << indent << "ScalarLabel: " << (this->ScalarLabel ? this->ScalarLabel : "(NULL)")
     << " - Value: " << this->Values[3] << endl;
}

//----------------------------------------------------------------------------
void vtkPVQuadViewInformation::Initialize()
{
  this->SetScalarLabel(NULL);
  this->SetXLabel(NULL);
  this->SetYLabel(NULL);
  this->SetZLabel(NULL);
  for(int i=0; i < 4; ++i)
    {
    this->Values[i] = -VTK_DOUBLE_MAX;
    }
}

//----------------------------------------------------------------------------
void vtkPVQuadViewInformation::CopyFromObject(vtkObject* object)
{
  vtkPVQuadRenderView* view = vtkPVQuadRenderView::SafeDownCast(object);

  if (!view)
    {
    return;
    }

  this->SetXLabel(view->GetXAxisLabel());
  this->SetYLabel(view->GetYAxisLabel());
  this->SetZLabel(view->GetZAxisLabel());
  this->SetScalarLabel(view->GetScalarLabel());
  double* origin = view->GetSliceOrigin(0);
  for(int i=0; i < 3; ++i)
    {
    this->Values[i] = origin[i];
    }
  this->Values[3] = view->GetScalarValue();
}

//----------------------------------------------------------------------------
void vtkPVQuadViewInformation::AddInformation(vtkPVInformation* info)
{
  vtkPVQuadViewInformation* other = vtkPVQuadViewInformation::SafeDownCast(info);
  if(info == NULL)
    {
    return;
    }

  if(this->XLabel == NULL)
    {
    this->SetXLabel(other->GetXLabel());
    }
  if(this->YLabel == NULL)
    {
    this->SetYLabel(other->GetYLabel());
    }
  if(this->ZLabel == NULL)
    {
    this->SetZLabel(other->GetZLabel());
    }
  if(this->ScalarLabel == NULL)
    {
    this->SetScalarLabel(other->GetScalarLabel());
    }
  for(int i=0; i < 4; ++i)
    {
    if(this->Values[i] == -VTK_DOUBLE_MAX)
      {
      this->Values[i] = other->Values[i];
      }
    }

}

//----------------------------------------------------------------------------
void vtkPVQuadViewInformation::CopyToStream(vtkClientServerStream* css)
{
  css->Reset();
  *css << vtkClientServerStream::Reply;
  *css << vtkClientServerStream::InsertArray(this->Values, 4)
       << this->XLabel
       << this->YLabel
       << this->ZLabel
       << this->ScalarLabel
       << vtkClientServerStream::End;
}

//----------------------------------------------------------------------------
void vtkPVQuadViewInformation::CopyFromStream(const vtkClientServerStream* css)
{
  this->Initialize();
  const char *tmp;
  if(!css->GetArgument(0, 0, this->Values, 4))
    {
    vtkErrorMacro("Error parsing values.");
    return;
    }

  if(!css->GetArgument(0, 1, &tmp))
    {
    vtkErrorMacro("Error parsing XLabel.");
    return;
    }
  this->SetXLabel(tmp);

  if(!css->GetArgument(0, 2, &tmp))
    {
    vtkErrorMacro("Error parsing YLabel.");
    return;
    }
  this->SetYLabel(tmp);

  if(!css->GetArgument(0, 3, &tmp))
    {
    vtkErrorMacro("Error parsing ZLabel.");
    return;
    }
  this->SetZLabel(tmp);

  if(!css->GetArgument(0, 4, &tmp))
    {
    vtkErrorMacro("Error parsing ScalarLabel.");
    return;
    }
  this->SetScalarLabel(tmp);
}
