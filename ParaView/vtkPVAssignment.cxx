/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVAssignment.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 1998-2000 Kitware Inc. 469 Clifton Corporate Parkway,
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
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVAssignment.h"

//----------------------------------------------------------------------------
vtkPVAssignment* vtkPVAssignment::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkPVAssignment");
  if(ret)
    {
    return (vtkPVAssignment*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkPVAssignment;
}

int vtkPVAssignmentCommand(ClientData cd, Tcl_Interp *interp,
                             int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVAssignment::vtkPVAssignment()
{  
  this->CommandFunction = vtkPVAssignmentCommand;
  this->Piece = 0;
  this->NumberOfPieces = 1;
  this->Extent[0] = this->Extent[2] = this->Extent[4] = 0;
  this->Extent[1] = this->Extent[3] = this->Extent[5] = 0;  

  this->Translator = vtkExtentTranslator::New();
  this->OriginalImage = NULL;
}

//----------------------------------------------------------------------------
vtkPVAssignment::~vtkPVAssignment()
{
  this->Translator->Delete();
  this->Translator = NULL;
  this->SetOriginalImage(NULL);
}

//----------------------------------------------------------------------------
void vtkPVAssignment::Clone(vtkPVApplication *pvApp)
{
  int num, id;
  
  if (this->Application)
    {
    vtkErrorMacro("Application has already been set.");
    }
  this->SetApplication(pvApp);

  // Clone this object on every other process, and set up the assignment.
  num = pvApp->GetController()->GetNumberOfProcesses();
  //this->SetPiece(0, num);
  this->SetPiece(0, 2);
  for (id = 1; id < num; ++id)
    {
    pvApp->RemoteScript(id, "%s %s", this->GetClassName(), this->GetTclName());
    pvApp->RemoteScript(id, "%s SetPiece %d %d", this->GetTclName(), id, num);
    }
}

//----------------------------------------------------------------------------
void vtkPVAssignment::SetOriginalImage(vtkPVImage *pvImage)
{
  vtkPVApplication *pvApp;
  
  if (this->OriginalImage == pvImage)
    {
    return;
    }
  
  this->Modified();
  
  pvApp = this->GetPVApplication();
  if (pvApp->GetController()->GetLocalProcessId() == 0)
    {
    pvApp->BroadcastScript("%s SetOriginalImage %s", this->GetTclName(),
			   pvImage->GetTclName());
    }
  
  if (this->OriginalImage)
    {
    vtkPVImage *tmp = this->OriginalImage;
    this->OriginalImage = NULL;
    tmp->UnRegister(this);
    }
  
  if (pvImage)
    {
    pvImage->Register(this);
    this->OriginalImage = pvImage;
    }
}

//----------------------------------------------------------------------------
void vtkPVAssignment::SetPiece(int piece, int numPieces)
{  
  if (this->Piece == piece && this->NumberOfPieces == numPieces)
    {
    return;
    }
  this->Modified();
  
  this->NumberOfPieces = numPieces;
  this->Piece = piece;
}



//----------------------------------------------------------------------------
int *vtkPVAssignment::GetExtent()
{
  vtkImageData *image;
  
  if (this->OriginalImage == NULL)
    {
    vtkErrorMacro("OriginalImage has not been set");
    return NULL;
    }

  image = this->OriginalImage->GetImageData();
  if (image == NULL)
    {
    vtkErrorMacro("OriginalImage has no data.");
    return NULL;
    }
  
  image->UpdateInformation();
  image->GetWholeExtent(this->Extent);
  
  this->Translator->SplitExtent(this->Piece, this->NumberOfPieces, this->Extent);
  
  return this->Extent;
}


//----------------------------------------------------------------------------
vtkPVApplication* vtkPVAssignment::GetPVApplication()
{
  if (this->Application == NULL)
    {
    return NULL;
    }
  
  if (this->Application->IsA("vtkPVApplication"))
    {  
    return (vtkPVApplication*)(this->Application);
    }
  else
    {
    vtkErrorMacro("Bad typecast");
    return NULL;
    } 
}


