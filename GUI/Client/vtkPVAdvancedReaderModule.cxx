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

#include "vtkCollectionIterator.h"
#include "vtkKWFrame.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVData.h"
#include "vtkPVFileEntry.h"
#include "vtkPVProcessModule.h"
#include "vtkPVWidgetCollection.h"
#include "vtkString.h"
#include "vtkVector.txx"
#include "vtkVectorIterator.txx"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVAdvancedReaderModule);
vtkCxxRevisionMacro(vtkPVAdvancedReaderModule, "1.21");

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
  vtkPVWidget *pvw;
  
  vtkCollection* widgets = this->GetWidgets();
  if (widgets)
    {
    vtkCollectionIterator *it = widgets->NewIterator();
    it->InitTraversal();

    for (int i = 0; i < widgets->GetNumberOfItems(); i++)
      {
      pvw = static_cast<vtkPVWidget*>(it->GetObject());
      pvw->ModifiedCallback();
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
