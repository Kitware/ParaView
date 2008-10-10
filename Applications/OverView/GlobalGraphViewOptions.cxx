/*=========================================================================

   Program: ParaView
   Module:    GlobalGraphViewOptions.cxx

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

/// \file GlobalGraphViewOptions.cxx
/// \date 7/20/2007

#include "GlobalGraphViewOptions.h"
#include "ui_GlobalGraphViewOptions.h"

#include <QPointer>

#include "vtkType.h"

#include "pqApplicationCore.h"
#include "pqPipelineRepresentation.h"  
#include "pqPluginManager.h"
//#include "ClientGraphView.h"
#include "pqServerManagerModel.h"
#include "pqSettings.h"
#include "pqViewModuleInterface.h"


GlobalGraphViewOptions::ManipulatorType GlobalGraphViewOptions::DefaultManipulatorTypes[] = 
{
    { 1, 0, 0, "Select"},
    { 2, 0, 0, "Pan"},
    { 3, 0, 0, "Zoom"},
    { 1, 1, 0, "Union Select"},
    { 2, 1, 0, "Pan"},
    { 3, 1, 0, "Zoom"},
    { 1, 0, 1, "Select"},
    { 2, 0, 1, "Pan"},
    { 3, 0, 1, "Zoom"},
};


class GlobalGraphViewOptions::pqInternal 
  : public Ui::GlobalGraphViewOptions
{
public:
  QList<QComboBox*> CameraControl3DComboBoxList;
  QList<QString> CameraControl3DComboItemList;
  QList<QComboBox*> CameraControl2DComboBoxList;
  QList<QString> CameraControl2DComboItemList;
};


//----------------------------------------------------------------------------
GlobalGraphViewOptions::GlobalGraphViewOptions(QWidget *widgetParent)
  : pqOptionsContainer(widgetParent)
{
  this->Internal = new pqInternal;
  this->Internal->setupUi(this);
  
  this->init();
}

GlobalGraphViewOptions::~GlobalGraphViewOptions()
{
  delete this->Internal;
}

void GlobalGraphViewOptions::init()
{
/*
  this->Internal->CameraControl3DComboBoxList << this->Internal->comboBoxCamera3D
      << this->Internal->comboBoxCamera3D_2 << this->Internal->comboBoxCamera3D_3
      << this->Internal->comboBoxCamera3D_4 << this->Internal->comboBoxCamera3D_5
      << this->Internal->comboBoxCamera3D_6 << this->Internal->comboBoxCamera3D_7
      << this->Internal->comboBoxCamera3D_8 << this->Internal->comboBoxCamera3D_9;
 
  this->Internal->CameraControl3DComboItemList //<< "FlyIn" << "FlyOut" << "Move"
     << "Pan" << "Roll" << "Rotate" << "Zoom";
  
  for ( int cc = 0; cc < this->Internal->CameraControl3DComboBoxList.size(); cc++ )
    {
    foreach(QString name, this->Internal->CameraControl3DComboItemList)
      {
      this->Internal->CameraControl3DComboBoxList.at(cc)->addItem(name);
      }
    }
*/

  this->Internal->CameraControl2DComboBoxList << this->Internal->comboBoxCamera2D
      << this->Internal->comboBoxCamera2D_2 << this->Internal->comboBoxCamera2D_3
      << this->Internal->comboBoxCamera2D_4 << this->Internal->comboBoxCamera2D_5
      << this->Internal->comboBoxCamera2D_6 << this->Internal->comboBoxCamera2D_7
      << this->Internal->comboBoxCamera2D_8 << this->Internal->comboBoxCamera2D_9;

  this->Internal->CameraControl2DComboItemList //<< "FlyIn" << "FlyOut" << "Move"
     << "Select" << "Pan" << "Zoom";

  for ( int cc = 0; cc < this->Internal->CameraControl2DComboBoxList.size(); cc++ )
    {
    foreach(QString name, this->Internal->CameraControl2DComboItemList)
      {
      this->Internal->CameraControl2DComboBoxList.at(cc)->addItem(name);
      }
    }

  // start fresh
  this->resetChanges();
/*
  for ( int cc = 0; cc < this->Internal->CameraControl3DComboBoxList.size(); cc++ )
    {
    QObject::connect(this->Internal->CameraControl3DComboBoxList[cc],
                    SIGNAL(currentIndexChanged(int)),
                    this, SIGNAL(changesAvailable()));
    }
*/
  for ( int cc = 0; cc < this->Internal->CameraControl2DComboBoxList.size(); cc++ )
    {
    QObject::connect(this->Internal->CameraControl2DComboBoxList[cc],
                    SIGNAL(currentIndexChanged(int)),
                    this, SIGNAL(changesAvailable()));
    }
  
  QObject::connect(this->Internal->resetCameraDefault,
    SIGNAL(clicked()), this, SLOT(resetDefaultCameraManipulators()));
  
}

void GlobalGraphViewOptions::setPage(const QString &page)
{
  if(page == "Graph View")
    {
    this->Internal->stackedWidget->setCurrentIndex(0);
    }

  QString which = page.section(".", 1, 1);

  int count = this->Internal->stackedWidget->count();
  for(int i=0; i<count; i++)
    {
    if(this->Internal->stackedWidget->widget(i)->objectName() == which)
      {
      this->Internal->stackedWidget->setCurrentIndex(i);
      break;
      }
    }
}

//-----------------------------------------------------------------------------
QStringList GlobalGraphViewOptions::getPageList()
{
  QStringList pages("Graph View");
  int count = this->Internal->stackedWidget->count();
  for(int i=0; i<count; i++)
    {
    pages << "Graph View." + this->Internal->stackedWidget->widget(i)->objectName();
    }
  return pages;
}
 
//-----------------------------------------------------------------------------
void GlobalGraphViewOptions::applyChanges()
{
  pqSettings* settings = pqApplicationCore::instance()->settings();
/*
  settings->beginGroup("graphView3D");
  
  // save out camera manipulators
  Manip manips[9];
  const Manip* default3DManips = ClientGraphView::getDefaultManipulatorTypes();
  for(int i=0; i<9; i++)
    {
    manips[i] = default3DManips[i];
    manips[i].Name =
      this->Internal->CameraControl3DComboBoxList[i]->currentText().toAscii();
    }
  
  QStringList strs;
  for(int i=0; i<9; i++)
    {
    strs << QString("Manipulator%1Mouse%2Shift%3Control%4Name%5")
                    .arg(i+1)
                    .arg(manips[i].Mouse)
                    .arg(manips[i].Shift)
                    .arg(manips[i].Control)
                    .arg(QString(manips[i].Name));
    }
  settings->setValue("InteractorStyle/CameraManipulators", strs);

  settings->endGroup();
*/
  // Now save out 2D camera manipulators (these are saved in a different group).
  settings->beginGroup("graphView2D");
  ManipulatorType manips[9];
  QStringList strs;
  for(int i=0; i<9; i++)
    {
    manips[i] = this->DefaultManipulatorTypes[i];
    manips[i].Name =
      this->Internal->CameraControl2DComboBoxList[i]->currentText().toAscii();
    }
  
  strs.clear();
  for(int i=0; i<9; i++)
    {
    strs << QString("Manipulator%1Mouse%2Shift%3Control%4Name%5")
                    .arg(i+1)
                    .arg(manips[i].Mouse)
                    .arg(manips[i].Shift)
                    .arg(manips[i].Control)
                    .arg(QString(manips[i].Name));
    }
  settings->setValue("InteractorStyle/CameraManipulators", strs);
  settings->endGroup();
/*
  // loop through render views and apply new settings
  QList<pqRenderView*> views =
    pqApplicationCore::instance()->getServerManagerModel()->
    findItems<pqRenderView*>();

  foreach(pqRenderView* view, views)
    {
    view->restoreSettings(true);
    }
*/
}

//-----------------------------------------------------------------------------
void GlobalGraphViewOptions::resetChanges()
{
  pqSettings* settings = pqApplicationCore::instance()->settings();

/*
  settings->beginGroup("graphView3D");

  val = settings->value("InteractorStyle/CameraManipulators");

  Manip manips[9];
  const Manip* default3DManips = ClientGraphView::getDefaultManipulatorTypes();
  for(int k = 0; k < 9; k++)
    {
    manips[k] = default3DManips[k];
    }

  if(val.isValid())
    {
    QStringList strs = val.toStringList();
    int num = strs.count();
    for(int i=0; i<num; i++)
      {
      Manip tmp;
      tmp.Name.resize(strs[i].size());
      int manip_index;
      if(5 == sscanf(strs[i].toAscii().data(),
        "Manipulator%dMouse%dShift%dControl%dName%s",
        &manip_index, &tmp.Mouse, &tmp.Shift, &tmp.Control, tmp.Name.data()))
        {
        for(int j=0; j<9; j++)
          {
          if(manips[j].Mouse == tmp.Mouse &&
             manips[j].Shift == tmp.Shift &&
             manips[j].Control == tmp.Control)
            {
            manips[j].Name = tmp.Name.data();
            break;
            }
          }
        }
      }
    }

  for(int i=0; i<9; i++)
    {
    int idx = this->Internal->CameraControl3DComboItemList.indexOf(manips[i].Name);
    this->Internal->CameraControl3DComboBoxList[i]->setCurrentIndex(idx);
    }

  settings->endGroup();
*/
  settings->beginGroup("graphView2D");
  QVariant val = settings->value("InteractorStyle/CameraManipulators");
  ManipulatorType manips[9];

  for(int k = 0; k < 9; k++)
    {
    manips[k] = this->DefaultManipulatorTypes[k];
    }

  if(val.isValid())
    {
    QStringList strs = val.toStringList();
    int num = strs.count();
    for(int i=0; i<num; i++)
      {
      ManipulatorType tmp;
      tmp.Name.resize(strs[i].size());
      int manip_index;
      if(5 == sscanf(strs[i].toAscii().data(),
        "Manipulator%dMouse%dShift%dControl%dName%s",
        &manip_index, &tmp.Mouse, &tmp.Shift, &tmp.Control, tmp.Name.data()))
        {
        for(int j=0; j<9; j++)
          {
          if(manips[j].Mouse == tmp.Mouse &&
             manips[j].Shift == tmp.Shift &&
             manips[j].Control == tmp.Control)
            {
            manips[j].Name = tmp.Name.data();
            break;
            }
          }
        }
      }
    }

  for(int i=0; i<9; i++)
    {
    int idx = this->Internal->CameraControl2DComboItemList.indexOf(manips[i].Name);
    this->Internal->CameraControl2DComboBoxList[i]->setCurrentIndex(idx);
    }
  settings->endGroup();
  
}

//-----------------------------------------------------------------------------
void GlobalGraphViewOptions::resetDefaultCameraManipulators()
{
/*
  const Manip* default3DManips = ClientGraphView::getDefaultManipulatorTypes();
  for(int i=0; i<9; i++)
    {
    int idx = this->Internal->CameraControl2DComboItemList.indexOf(
      default2DManips[i].Name);
    this->Internal->CameraControl2DComboBoxList[i]->setCurrentIndex(idx);
    }
*/
/*
  const Manip* default2DManips = ClientGraphView::getDefaultManipulatorTypes();
  for(int i=0; i<9; i++)
    {
    int idx = this->Internal->CameraControl2DComboItemList.indexOf(
      default2DManips[i].Name);
    this->Internal->CameraControl2DComboBoxList[i]->setCurrentIndex(idx);
    }
*/
}
