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

#include "vtkCacheSizeKeeper.h"
#include "vtkClientConnection.h"
#include "vtkCommandOptions.h"
#include "vtkCommandOptionsXMLParser.h"
#include "vtkConnectionIterator.h"
#include "vtkMPIMToNSocketConnection.h"
#include "vtkMPIMToNSocketConnectionPortInformation.h"
#include "vtkMPISelfConnection.h"
#include "vtkProcessModule.h"
#include "vtkProcessModuleConnectionManager.h"
#include "vtkProcessModuleGUIHelper.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVCacheSizeInformation.h"
#include "vtkPVClassNameInformation.h"
#include "vtkPVClientServerIdCollectionInformation.h"
#include "vtkPVCompositeDataInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkPVDisplayInformation.h"
#include "vtkPVEnvironmentInformation.h"
#include "vtkPVEnvironmentInformationHelper.h"
#include "vtkPVFileInformation.h"
#include "vtkPVFileInformationHelper.h"
#include "vtkPVGenericAttributeInformation.h"
#include "vtkPVInformation.h"
#include "vtkPVAlgorithmPortsInformation.h"
#include "vtkPVOpenGLExtensionsInformation.h"
#include "vtkPVOptions.h"
#include "vtkPVOptionsXMLParser.h"
#include "vtkPVPluginLoader.h"
#include "vtkPVProgressHandler.h"
#include "vtkPVSelectionInformation.h"
#include "vtkPVServerInformation.h"
#include "vtkPVServerOptions.h"
#include "vtkPVServerSocket.h"
#include "vtkPVTimerInformation.h"
#include "vtkPVXMLElement.h"
#include "vtkPVXMLParser.h"
#include "vtkRemoteConnection.h"
#include "vtkSelectionSerializer.h"
#include "vtkSelfConnection.h"
#include "vtkServerConnection.h"
#include "vtkStringList.h"
#include "vtkUndoSet.h"
#include "vtkUndoStack.h"

int main(int, char * [])
{
  vtkObject *c;

  c = vtkMPIMToNSocketConnection::New(); c->Print(cout); c->Delete();
  c = vtkPVCacheSizeInformation::New(); c->Print(cout); c->Delete();
  c = vtkPVClassNameInformation::New(); c->Print(cout); c->Delete();
  c = vtkPVClientServerIdCollectionInformation::New(); c->Print(cout); c->Delete();
  c = vtkMPIMToNSocketConnectionPortInformation::New(); c->Print(cout); c->Delete();
  c = vtkPVServerInformation::New(); c->Print(cout); c->Delete();
  c = vtkPVServerOptions::New(); c->Print(cout); c->Delete();
  c = vtkProcessModuleGUIHelper::New(); c->Print(cout); c->Delete();
  c = vtkPVDataInformation::New(); c->Print(cout); c->Delete();
  c = vtkPVAlgorithmPortsInformation::New(); c->Print(cout); c->Delete();
  c = vtkPVTimerInformation::New(); c->Print(cout); c->Delete();
  c = vtkPVXMLElement::New(); c->Print(cout); c->Delete();
  c = vtkPVXMLParser::New(); c->Print(cout); c->Delete();
  c = vtkProcessModule::New(); c->Print(cout); c->Delete();
  c = vtkPVDataSetAttributesInformation::New(); c->Print(cout); c->Delete();
  c = vtkPVDisplayInformation::New(); c->Print(cout); c->Delete();
  c = vtkPVEnvironmentInformation::New(); c->Print(cout); c->Delete();
  c = vtkPVEnvironmentInformationHelper::New(); c->Print(cout); c->Delete();
  c = vtkStringList::New(); c->Print(cout); c->Delete();
  c = vtkPVArrayInformation::New(); c->Print(cout); c->Delete();
  c = vtkPVGenericAttributeInformation::New(); c->Print(cout); c->Delete();
  c = vtkPVFileInformation::New(); c->Print(cout); c->Delete();
  c = vtkPVFileInformationHelper::New(); c->Print(cout); c->Delete();
  c = vtkPVInformation::New(); c->Print(cout); c->Delete();
  c = vtkPVPluginLoader::New(); c->Print( cout ); c->Delete();
  c = vtkPVProgressHandler::New(); c->Print(cout); c->Delete();
  c = vtkPVOptions::New(); c->Print(cout); c->Delete();
  c = vtkPVOptionsXMLParser::New(); c->Print(cout); c->Delete();
  c = vtkPVOpenGLExtensionsInformation::New(); c->Print(cout); c->Delete();
  c = vtkPVSelectionInformation::New(); c->Print(cout); c->Delete();
  c = vtkPVCompositeDataInformation::New(); c->Print(cout); c->Delete();

  c = vtkCommandOptions::New(); c->Print(cout); c->Delete();
  c = vtkCommandOptionsXMLParser::New(); c->Print(cout); c->Delete();
  c = vtkCacheSizeKeeper::New(); c->Print(cout); c->Delete();
  c = vtkClientConnection::New(); c->Print(cout); c->Delete();
  c = vtkConnectionIterator::New(); c->Print(cout); c->Delete();
  c = vtkMPISelfConnection::New(); c->Print(cout); c->Delete();
  c = vtkPVServerSocket::New(); c->Print(cout); c->Delete();
  c = vtkProcessModuleConnectionManager::New(); c->Print(cout); c->Delete();
  c = vtkRemoteConnection::New(); c->Print(cout); c->Delete();
  c = vtkSelectionSerializer::New(); c->Print(cout); c->Delete();
  c = vtkSelfConnection::New(); c->Print(cout); c->Delete();
  c = vtkServerConnection::New(); c->Print(cout); c->Delete();
  c = vtkUndoSet::New(); c->Print(cout); c->Delete();
  c = vtkUndoStack::New(); c->Print(cout); c->Delete();
  return 0;
}
