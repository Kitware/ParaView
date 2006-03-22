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

#include "vtkClientConnection.h"
#include "vtkConnectionIterator.h"
#include "vtkMPIMToNSocketConnection.h"
#include "vtkMPIMToNSocketConnectionPortInformation.h"
#include "vtkMPISelfConnection.h"
#include "vtkProcessModule.h"
#include "vtkProcessModuleConnectionManager.h"
#include "vtkProcessModuleGUIHelper.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVClassNameInformation.h"
#include "vtkPVCompositeDataInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkPVInformation.h"
#include "vtkPVLODPartDisplayInformation.h"
#include "vtkPVNumberOfOutputsInformation.h"
#include "vtkPVOptions.h"
#include "vtkPVProgressHandler.h"
#include "vtkPVServerInformation.h"
#include "vtkPVServerSocket.h"
#include "vtkPVTimerInformation.h"
#include "vtkRemoteConnection.h"
#include "vtkSelfConnection.h"
#include "vtkServerConnection.h"
#include "vtkStringList.h"

int main(int, char * [])
{
  vtkObject *c;

  c = vtkMPIMToNSocketConnection::New(); c->Print(cout); c->Delete();
  c = vtkPVClassNameInformation::New(); c->Print(cout); c->Delete();
  c = vtkPVLODPartDisplayInformation::New(); c->Print(cout); c->Delete();
  c = vtkMPIMToNSocketConnectionPortInformation::New(); c->Print(cout); c->Delete();
  c = vtkPVServerInformation::New(); c->Print(cout); c->Delete();
  c = vtkProcessModuleGUIHelper::New(); c->Print(cout); c->Delete();
  c = vtkPVDataInformation::New(); c->Print(cout); c->Delete();
  c = vtkPVNumberOfOutputsInformation::New(); c->Print(cout); c->Delete();
  c = vtkPVTimerInformation::New(); c->Print(cout); c->Delete();
  c = vtkProcessModule::New(); c->Print(cout); c->Delete();
  c = vtkPVDataSetAttributesInformation::New(); c->Print(cout); c->Delete();
  c = vtkStringList::New(); c->Print(cout); c->Delete();
  c = vtkPVArrayInformation::New(); c->Print(cout); c->Delete();
  c = vtkPVInformation::New(); c->Print(cout); c->Delete();
  c = vtkPVProgressHandler::New(); c->Print(cout); c->Delete();
  c = vtkPVOptions::New(); c->Print(cout); c->Delete();
  c = vtkPVCompositeDataInformation::New(); c->Print(cout); c->Delete();

  c = vtkClientConnection::New(); c->Print(cout); c->Delete();
  c = vtkConnectionIterator::New(); c->Print(cout); c->Delete();
  c = vtkMPISelfConnection::New(); c->Print(cout); c->Delete();
  c = vtkPVServerSocket::New(); c->Print(cout); c->Delete();
  c = vtkProcessModuleConnectionManager::New(); c->Print(cout); c->Delete();
  c = vtkRemoteConnection::New(); c->Print(cout); c->Delete();
  c = vtkSelfConnection::New(); c->Print(cout); c->Delete();
  c = vtkServerConnection::New(); c->Print(cout); c->Delete();
  return 0;
}
