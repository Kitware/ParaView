/*=========================================================================

   Program: ParaView
   Module:    pqLookmarkModel.cxx

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
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

/// \file pqLookmarkModel.cxx
/// \date 4/14/2006

#include "pqLookmarkModel.h"

// ParaView includes.
#include "vtkPVXMLElement.h"
#include "vtkPVXMLParser.h"
#include "vtkSmartPointer.h"
#include "vtkSMPQStateLoader.h"
#include "vtkSMProxyLocator.h"
#include "vtkSMStateLoader.h"
#include "vtksys/ios/sstream"

#include "pqApplicationCore.h"
#include "pqRepresentation.h"
#include "pqLookmarkStateLoader.h"
#include "pqObjectBuilder.h"
#include "pqPipelineFilter.h"
#include "pqRenderView.h"
#include "pqServer.h"

#include <QBuffer>
#include <QByteArray>
#include <QImage>
#include <QPixmap>
#include <QtDebug>

//-----------------------------------------------------------------------------
pqLookmarkModel::pqLookmarkModel(QString name, const QString &state, QObject* _parent /*=null*/)
  : QObject(_parent)
{
  this->Name = name;
  this->RestoreTime = false;
  this->RestoreCamera = false;
  this->RestoreData = false;
  this->State = state;
  this->PipelineHierarchy = 0;
}

pqLookmarkModel::pqLookmarkModel(const pqLookmarkModel &other,
    QObject *parentObject)
  : QObject(parentObject)
{
  vtkPVXMLElement *root = vtkPVXMLElement::New();
  other.saveState(root);
  this->initializeState(root);
  root->Delete();
}

pqLookmarkModel::pqLookmarkModel(vtkPVXMLElement *lookmark,
    QObject *parentObject)
  : QObject(parentObject)
{
  this->initializeState(lookmark);
}

void pqLookmarkModel::initializeState(vtkPVXMLElement *lookmark)
{
  // REQUIRED PROPERTIES: name and state
 
  const char *tempName = lookmark->GetAttribute("Name");
  this->Name = tempName;

  vtkPVXMLElement *stateRoot = lookmark->FindNestedElementByName("ServerManagerState");
  if(!stateRoot)
    {
    return;
    }
  // convert state xml to a qstring
  vtksys_ios::ostringstream stateStream;
  stateRoot->PrintXML(stateStream, vtkIndent(0));
  stateStream << ends;
  this->State = stateStream.str().c_str();

  // OPTIONAL PROPERTIES: 

  this->PipelineHierarchy = lookmark->FindNestedElementByName("PipelineHierarchy");

  int val;
  if(lookmark->GetScalarAttribute("RestoreData",&val))
     this->RestoreData = val;
  if(lookmark->GetScalarAttribute("RestoreCamera",&val))
    this->RestoreCamera = val;
  if(lookmark->GetScalarAttribute("RestoreTime",&val))
    this->RestoreTime = val;

  const char *tempDesc = lookmark->GetAttribute("Comments");
  this->Description = tempDesc;

  vtkPVXMLElement *iconElement = lookmark->FindNestedElementByName("Icon");
  if(iconElement)
    {
    QByteArray array(iconElement->GetAttribute("Value"));
    this->Icon.loadFromData(QByteArray::fromBase64(array),"PNG");
    }

  emit this->modified(this);
}


QString pqLookmarkModel::getState() const
{
  return this->State;
}

vtkPVXMLElement* pqLookmarkModel::getPipelineHierarchy() const
{
  if(this->PipelineHierarchy)
    {
    return this->PipelineHierarchy; 
    }
  return 0;
}


void pqLookmarkModel::setName(QString newName)
{
  QString oldName = this->Name;

  this->Name = newName;

  if(QString::compare(oldName,newName) != 0)
    {
    emit this->nameChanged(oldName,newName);
    emit this->modified(this);
    }
}

void pqLookmarkModel::setState(QString state)
{
  this->State = state;
  emit this->modified(this);
}

void pqLookmarkModel::setRestoreTimeFlag(bool state)
{
  this->RestoreTime = state;
  emit this->modified(this);
}


void pqLookmarkModel::setRestoreDataFlag(bool state)
{
  this->RestoreData = state;
  emit this->modified(this);
}

void pqLookmarkModel::setRestoreCameraFlag(bool state)
{
  this->RestoreCamera = state;
  emit this->modified(this);
}


void pqLookmarkModel::setDescription(QString text)
{
  this->Description = text;
  emit this->modified(this);
}


void pqLookmarkModel::setIcon(QImage icon)
{
  this->Icon = icon;
  emit this->modified(this);
}

void pqLookmarkModel::setPipelineHierarchy(vtkPVXMLElement *pipeline)
{
  this->PipelineHierarchy = pipeline;
  emit this->modified(this);
}


void pqLookmarkModel::load(
              pqServer *server, 
              QList<pqPipelineSource*> *sources, 
              pqView *view,  
              vtkSMStateLoader *arg_loader)
{
  if(!server)
    {
    qDebug() << "Cannot load lookmark without an active server";
    return;
    }

  if(!view)
    {
    qDebug() << "Cannot load lookmark without a valid view";
    return;
    }

  // Now deal with the different types of possible state loaders:
  vtkSmartPointer<vtkSMStateLoader> loader = arg_loader;
  if (!loader)
    {
    loader.TakeReference(pqLookmarkStateLoader::New());
    }

  bool resetCamera = false;
  // remember to reset the camera later if the view has no displays visible
  if(view->getNumberOfVisibleRepresentations()==0 && !this->RestoreCamera)
    {
    resetCamera = true;
    }


  // Now turn off visibility of all displays currently added to view.
  // We do this before the lookmark is loaded so that other sources
  // do not obstruct the view of the lookmark sources.
  QList<pqRepresentation*> displays = view->getRepresentations();
  for(int i=0; i<displays.count(); i++)
    {
    pqRepresentation *disp = displays[i];
    disp->setVisible(0);
    }

  // If this is a lookmark of a single view, the active view needs to be 
  // added to the beginning of the loader's preferred view list to ensure 
  // it is used before any others
  vtkSMPQStateLoader* smpqLoader = vtkSMPQStateLoader::SafeDownCast(loader);
  if (smpqLoader)
    {
    smpqLoader->AddPreferredView(view->getViewProxy());
    }

  // set some parameters specific to the lookmark state loader
  pqLookmarkStateLoader *pqLoader = pqLookmarkStateLoader::SafeDownCast(loader);
  if(pqLoader)
    {
    pqLoader->SetPreferredSources(sources);
    pqLoader->SetRestoreCameraFlag(this->RestoreCamera);
    pqLoader->SetRestoreTimeFlag(this->RestoreTime);
    pqLoader->SetPipelineHierarchy(this->PipelineHierarchy);
    pqLoader->SetTimeKeeper(server->getTimeKeeper());
    pqLoader->SetView(view);
    }

  // convert the stored state from a qstring to a vtkPVXMLElement
  vtkPVXMLParser *parser = vtkPVXMLParser::New();
  parser->Parse(this->State.toAscii().data());
  vtkPVXMLElement *stateElement = parser->GetRootElement();
  if(!stateElement)
    {
    qDebug() << "Could not parse lookmark's state.";
    parser->Delete();
    return;
    }

  pqLoader->GetProxyLocator()->SetConnectionID(server->GetConnectionID());
  pqLoader->LoadState(stateElement);

  // If this is a render module with no previous visible representations
  // and RestoreCamera is turned off, reset the camera.
  pqRenderView* renModule = qobject_cast<pqRenderView*>(view);
  if(resetCamera && renModule)
    {
    renModule->resetCamera();
    renModule->render();
    }

  parser->Delete();
  emit this->loaded(this);
}



void pqLookmarkModel::saveState(vtkPVXMLElement *lookmark) const
{
  lookmark->AddAttribute("Name", this->getName().toAscii().constData());
  lookmark->AddAttribute("RestoreData", this->getRestoreDataFlag());
  lookmark->AddAttribute("RestoreCamera", this->getRestoreCameraFlag());
  lookmark->AddAttribute("RestoreTime", this->getRestoreTimeFlag());

  // convert the stored state from a qstring to a vtkPVXMLElement
  vtkPVXMLParser *parser = vtkPVXMLParser::New();
  parser->Parse(this->State.toAscii().data());
  vtkPVXMLElement *stateElement = parser->GetRootElement();
  if(!stateElement)
    {
    qDebug() << "Could not parse lookmark's state.";
    parser->Delete();
    return;
    }

  lookmark->AddNestedElement(stateElement); 

  if(this->PipelineHierarchy) 
    {
    lookmark->AddNestedElement(this->PipelineHierarchy); 
    }

  if(!this->Description.isEmpty() && !this->Description.isNull())
    {
    lookmark->AddAttribute("Comments", this->getDescription().toAscii().constData());
    }

  // Icon
  if(!this->Icon.isNull())
    {
    QByteArray ba;
    QBuffer buffer(&ba);
    buffer.open(QIODevice::WriteOnly);
    QImage image = this->getIcon();
    image.save(&buffer, "PNG"); // writes image into ba in PNG format
    ba = ba.toBase64();
    vtkPVXMLElement *iconElement = vtkPVXMLElement::New();
    iconElement->SetName("Icon");
    iconElement->AddAttribute("Value",ba.constData());
    lookmark->AddNestedElement(iconElement);
    iconElement->Delete();
    }

  parser->Delete();
}


