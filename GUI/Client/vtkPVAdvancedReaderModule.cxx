/*=========================================================================

  Program:   ParaView
  Module:    vtkPVAdvancedReaderModule.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVAdvancedReaderModule.h"

#include "vtkCollection.h"
#include "vtkCollectionIterator.h"
#include "vtkKWFrame.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVData.h"
#include "vtkPVFileEntry.h"
#include "vtkPVProcessModule.h"
#include "vtkPVWidgetProperty.h"
#include "vtkString.h"
#include "vtkVector.txx"
#include "vtkVectorIterator.txx"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVAdvancedReaderModule);
vtkCxxRevisionMacro(vtkPVAdvancedReaderModule, "1.17");

int vtkPVAdvancedReaderModuleCommand(ClientData cd, Tcl_Interp *interp,
                        int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVAdvancedReaderModule::vtkPVAdvancedReaderModule()
{
  this->CommandFunction = vtkPVAdvancedReaderModuleCommand;
  this->AcceptAfterRead = 0;
}

//----------------------------------------------------------------------------
vtkPVAdvancedReaderModule::~vtkPVAdvancedReaderModule()
{
}

//----------------------------------------------------------------------------
// This method used to fix the output data type of clone.
// It does nothing now, so we should get rid of it........ !!!!!!!!
int vtkPVAdvancedReaderModule::Initialize(const char* fname, 
                                          vtkPVReaderModule*& clone)
{
  int retVal = this->Superclass::Initialize(fname, clone);

  if (retVal != VTK_OK)
    {
    return retVal;
    }
  return VTK_OK;
}

//----------------------------------------------------------------------------
int vtkPVAdvancedReaderModule::ReadFileInformation(const char* fname)
{
  int retVal =  this->Superclass::ReadFileInformation(fname);
  if (retVal != VTK_OK)
    {
    return retVal;
    }
  
  // We need to update the widgets.
  vtkPVWidgetProperty *pvwProp;
  
  vtkCollection* props = this->GetWidgetProperties();
  if (props)
    {
    vtkCollectionIterator *it = props->NewIterator();
    it->InitTraversal();

    for (int i = 0; i < props->GetNumberOfItems(); i++)
      {
      pvwProp = static_cast<vtkPVWidgetProperty*>(it->GetObject());
      pvwProp->GetWidget()->ModifiedCallback();
      it->GoToNextItem();
      }
    it->Delete();
    this->UpdateParameterWidgets();
    }

  return VTK_OK;
}

//----------------------------------------------------------------------------
int vtkPVAdvancedReaderModule::Finalize(const char* fname)
{
  return this->FinalizeInternal(fname, 0);
}

//----------------------------------------------------------------------------
void vtkPVAdvancedReaderModule::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
