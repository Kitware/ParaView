/*=========================================================================

  Program:   ParaView
  Module:    vtkSMEnvironmentAnnotationProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMEnvironmentAnnotationProxy.h"

#include "vtkClientServerStream.h"
#include "vtkCollection.h"
#include "vtkCommand.h"
#include "vtkDataSetAttributes.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkPVAlgorithmPortsInformation.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkPVXMLElement.h"
#include "vtkSmartPointer.h"
#include "vtkSMDocumentation.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMIdTypeVectorProperty.h"
#include "vtkSMInputProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMMessage.h"
#include "vtkSMOutputPort.h"
#include "vtkSMProxyLocator.h"
#include "vtkSMSession.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMStringVectorProperty.h"

#include <string>
#include <vector>
#include <vtksys/ios/sstream>
#include <assert.h>

//---------------------------------------------------------------------------
vtkStandardNewMacro(vtkSMEnvironmentAnnotationProxy);

//---------------------------------------------------------------------------
vtkSMEnvironmentAnnotationProxy::vtkSMEnvironmentAnnotationProxy()
{
}

//---------------------------------------------------------------------------
vtkSMEnvironmentAnnotationProxy::~vtkSMEnvironmentAnnotationProxy()
{
}

//---------------------------------------------------------------------------
void vtkSMEnvironmentAnnotationProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//---------------------------------------------------------------------------
void vtkSMEnvironmentAnnotationProxy::UpdateVTKObjects()
{
  this->Superclass::UpdateVTKObjects();
}