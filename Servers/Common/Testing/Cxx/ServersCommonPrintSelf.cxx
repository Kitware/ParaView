/*=========================================================================

  Program:   ParaView
  Module:    ServersCommonPrintSelf.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkMPIMToNSocketConnection.h"
#include "vtkPVClassNameInformation.h"
#include "vtkPVLODPartDisplayInformation.h"
#include "vtkPVRenderModule.h"
#include "vtkMPIMToNSocketConnectionPortInformation.h"
#include "vtkPVClientServerModule.h"
#include "vtkPVMPIProcessModule.h"
#include "vtkPVServerInformation.h"
#include "vtkProcessModuleGUIHelper.h"
#include "vtkPVDataInformation.h"
#include "vtkPVNumberOfOutputsInformation.h"
#include "vtkPVTimerInformation.h"
#include "vtkProcessModule.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkPVProcessModule.h"
#include "vtkStringList.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVInformation.h"
#include "vtkPVProgressHandler.h"
#include "vtkPVOptions.h"

int main(int, char * [])
{
  vtkObject *c;

  c = vtkMPIMToNSocketConnection::New(); c->Print(cout); c->Delete();
  c = vtkPVClassNameInformation::New(); c->Print(cout); c->Delete();
  c = vtkPVLODPartDisplayInformation::New(); c->Print(cout); c->Delete();
  c = vtkPVRenderModule::New(); c->Print(cout); c->Delete();
  c = vtkMPIMToNSocketConnectionPortInformation::New(); c->Print(cout); c->Delete();
  c = vtkPVClientServerModule::New(); c->Print(cout); c->Delete();
  c = vtkPVMPIProcessModule::New(); c->Print(cout); c->Delete();
  c = vtkPVServerInformation::New(); c->Print(cout); c->Delete();
  c = vtkProcessModuleGUIHelper::New(); c->Print(cout); c->Delete();
  c = vtkPVDataInformation::New(); c->Print(cout); c->Delete();
  c = vtkPVNumberOfOutputsInformation::New(); c->Print(cout); c->Delete();
  c = vtkPVTimerInformation::New(); c->Print(cout); c->Delete();
  c = vtkProcessModule::New(); c->Print(cout); c->Delete();
  c = vtkPVDataSetAttributesInformation::New(); c->Print(cout); c->Delete();
  c = vtkPVProcessModule::New(); c->Print(cout); c->Delete();
  c = vtkStringList::New(); c->Print(cout); c->Delete();
  c = vtkPVArrayInformation::New(); c->Print(cout); c->Delete();
  c = vtkPVInformation::New(); c->Print(cout); c->Delete();
  c = vtkPVProgressHandler::New(); c->Print(cout); c->Delete();
  c = vtkPVOptions::New(); c->Print(cout); c->Delete();

  return 0;
}
