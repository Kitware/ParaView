/*=========================================================================

   Program: ParaView
   Module: pqStandardPropertyWidgetInterface.cxx

   Copyright (c) 2012 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2.

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/

#include "pqStandardPropertyWidgetInterface.h"

#include "pqApplicationCore.h"
#include "pqArrayStatusPropertyWidget.h"
#include "pqColorEditorPropertyWidget.h"
#include "pqCubeAxesPropertyWidget.h"
#include "pqDisplayRepresentationWidget.h"
#include "pqPipelineRepresentation.h"
#include "pqProxy.h"
#include "pqServerManagerModel.h"
#include "vtkSMPropertyGroup.h"
#include "vtkSMProperty.h"
#include "vtkSMProxy.h"

pqStandardPropertyWidgetInterface::pqStandardPropertyWidgetInterface(QObject *p)
  : QObject(p)
{
}

pqStandardPropertyWidgetInterface::~pqStandardPropertyWidgetInterface()
{
}

pqPropertyWidget*
pqStandardPropertyWidgetInterface::createWidgetForProperty(vtkSMProxy *proxy,
                                                           vtkSMProperty *property)
{
  pqServerManagerModel *smm = pqApplicationCore::instance()->getServerManagerModel();
  pqPipelineRepresentation* repr = smm->findItem<pqPipelineRepresentation *>(proxy);
  if(repr && std::string(proxy->GetPropertyName(property)) == "Representation")
    {
    return new pqDisplayRepresentationPropertyWidget(proxy);
    }

//  if(propert = "textture")
//    {
//    pqTextureComboBox;
//    }

  return 0;
}

pqPropertyWidget*
pqStandardPropertyWidgetInterface::createWidgetForPropertyGroup(vtkSMProxy *proxy,
                                                                vtkSMPropertyGroup *group)
{
  if(QString(group->GetType()) == "ColorEditor")
    {
    return new pqColorEditorPropertyWidget(proxy);
    }
  else if(QString(group->GetType()) == "CubeAxes")
    {
    return new pqCubeAxesPropertyWidget(proxy);
    }
  else if (QString(group->GetType()) == "ArrayStatus")
    {
    return new pqArrayStatusPropertyWidget(proxy, group);
    }

  return 0;
}
