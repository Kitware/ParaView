/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVWidget.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 2000-2001 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither the name of Kitware nor the names of any contributors may be used
   to endorse or promote products derived from this software without specific 
   prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkPVWidget.h"
#include "vtkObjectFactory.h"
#include "vtkKWApplication.h"
#include "vtkKWEvent.h"
#include "vtkCollection.h"

//----------------------------------------------------------------------------
vtkPVWidget* vtkPVWidget::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkPVWidget");
  if (ret)
    {
    return (vtkPVWidget*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkPVWidget;
}

//----------------------------------------------------------------------------
vtkPVWidget::vtkPVWidget()
{
  this->ModifiedCommandObjectTclName = NULL;
  this->ModifiedCommandMethod = NULL;

  // Start modified because empty widgets do not match their variables.
  this->ModifiedFlag = 1;

  this->DependantCollection = vtkCollection::New();
}

//----------------------------------------------------------------------------
vtkPVWidget::~vtkPVWidget()
{
  this->SetModifiedCommandObjectTclName(NULL);
  this->SetModifiedCommandMethod(NULL);

  this->DependantCollection->Delete();
  this->DependantCollection = NULL;
}

//----------------------------------------------------------------------------
void vtkPVWidget::SetModifiedCommand(const char* cmdObject, 
                                     const char* methodAndArgs)
{
  this->SetModifiedCommandObjectTclName(cmdObject);
  this->SetModifiedCommandMethod(methodAndArgs);
}

//----------------------------------------------------------------------------
void vtkPVWidget::Accept()
{
  this->ModifiedFlag = 0;
}

//----------------------------------------------------------------------------
void vtkPVWidget::Reset()
{
  this->Update();
  // We want the modifiedCallbacks to occur before we reset this flag.
  this->Script("update");
  this->ModifiedFlag = 0;
}

//----------------------------------------------------------------------------
void vtkPVWidget::Update()
{
  vtkPVWidget *pvw;
  
  this->DependantCollection->InitTraversal();
  while ( (pvw = (vtkPVWidget*)(this->DependantCollection->GetNextItemAsObject())) )
    {
    pvw->Update();
    }  
}

//----------------------------------------------------------------------------
void vtkPVWidget::AddDependant(vtkPVWidget *pvw)
{
  this->DependantCollection->AddItem(pvw);  
}


//----------------------------------------------------------------------------
void vtkPVWidget::ModifiedCallback()
{
  this->ModifiedFlag = 1;
  
  if (this->ModifiedCommandObjectTclName && this->ModifiedCommandMethod)
    {
    this->Script("%s %s", this->ModifiedCommandObjectTclName,
                 this->ModifiedCommandMethod);
    }
}

//----------------------------------------------------------------------------
void vtkPVWidget::SaveInTclScript(ofstream *file, const char *sourceName)
{
  file = file;
  sourceName = sourceName;
}



 
