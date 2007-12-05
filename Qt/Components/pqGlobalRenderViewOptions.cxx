/*=========================================================================

   Program: ParaView
   Module:    pqGlobalRenderViewOptions.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.1. 

   See License_v1.1.txt for the full ParaView license.
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

/// \file pqGlobalRenderViewOptions.cxx
/// \date 7/20/2007

#include "pqGlobalRenderViewOptions.h"
#include "ui_pqGlobalRenderViewOptions.h"

#include <QPointer>

#include "vtkType.h"

#include "pqApplicationCore.h"
#include "pqPipelineRepresentation.h"  
#include "pqPluginManager.h"
#include "pqRenderView.h"
#include "pqServerManagerModel.h"
#include "pqSettings.h"
#include "pqViewModuleInterface.h"

struct Manip
{
  int Mouse;
  int Shift;
  int Control;
  QByteArray Name;

  Manip &operator=(const Manip &other)
    {
    this->Mouse = other.Mouse;
    this->Shift = other.Shift;
    this->Control = other.Control;
    this->Name = other.Name;
    return *this;
    }
};

// set up with default names
static Manip DefaultManips[9] =
  {
    { 1, 0, 0, "Rotate"},
    { 2, 0, 0, "Pan"},
    { 3, 0, 0, "Zoom"},
    { 1, 1, 0, "Roll"},
    { 2, 1, 0, "Rotate"},
    { 3, 1, 0, "Pan"},
    { 1, 0, 1, "Zoom"},
    { 2, 0, 1, "Rotate"},
    { 3, 0, 1, "Zoom"},
  };

class pqGlobalRenderViewOptions::pqInternal 
  : public Ui::pqGlobalRenderViewOptions
{
public:
  QList<QComboBox*> CameraControl3DComboBoxList;
  QList<QString> CameraControl3DComboItemList;

  void updateLODThresholdLabel(int value)
    {
    this->lodThresholdLabel->setText(
      QString("%1").arg(value/10.0, 0, 'f', 2) + " MBytes");
    }
  void updateLODResolutionLabel(int value)
    {
    QVariant val(160-value + 10);

    this->lodResolutionLabel->setText(
      val.toString() + "x" + val.toString() + "x" + val.toString());
    }
  void updateOutlineThresholdLabel(int value)
    {
    this->outlineThresholdLabel->setText(
      QVariant(value/10.0).toString() + " MCells");
    }
  
  void updateCompositeThresholdLabel(int value)
    {
    this->compositeThresholdLabel->setText(
      QVariant(value/10.0).toString() + " MBytes");
    }
  void updateSubsamplingRateLabel(int value)
    {
    this->subsamplingRateLabel->setText(QVariant(value).toString() 
      + " Pixels");
    }
  void updateSquirtLevelLabel(int val)
    {
    static int bitValues[] = {24, 24, 22, 19, 16, 13, 10};
    val = (val < 0 )? 0 : val;
    val = ( val >6)? 6 : val;
    this->squirtLevelLabel->setText(
      QVariant(bitValues[val]).toString() + " Bits");
    }

  void updateStillSubsampleRateLabel(int value)
    {
    if (value == 1)
      {
      this->stillRenderSubsampleRateLabel->setText("Disabled");
      }
    else
      {
      this->stillRenderSubsampleRateLabel->setText(
        QString("%1 Pixels").arg(value));
      }
    }

  void updateClientCollectLabel(double value_in_mb)
    {
    this->clientCollectLabel->setText(
      QString("%1 MBytes").arg(value_in_mb));
    }
};


//----------------------------------------------------------------------------
pqGlobalRenderViewOptions::pqGlobalRenderViewOptions(QWidget *widgetParent)
  : pqOptionsContainer(widgetParent)
{
  this->Internal = new pqInternal;
  this->Internal->setupUi(this);
  
  this->init();
}

pqGlobalRenderViewOptions::~pqGlobalRenderViewOptions()
{
  delete this->Internal;
}

void pqGlobalRenderViewOptions::init()
{
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

  // start fresh
  this->resetChanges();

  QObject::connect(this->Internal->lodThreshold,
    SIGNAL(valueChanged(int)), this, SLOT(lodThresholdSliderChanged(int)));

  QObject::connect(this->Internal->lodResolution,
    SIGNAL(valueChanged(int)), this, SLOT(lodResolutionSliderChanged(int)));

  QObject::connect(this->Internal->outlineThreshold,
    SIGNAL(valueChanged(int)), this, SLOT(outlineThresholdSliderChanged(int)));

  QObject::connect(this->Internal->compositeThreshold,
    SIGNAL(valueChanged(int)), this, SLOT(compositeThresholdSliderChanged(int)));

  QObject::connect(this->Internal->subsamplingRate,
    SIGNAL(valueChanged(int)), this, SLOT(subsamplingRateSliderChanged(int)));

  QObject::connect(this->Internal->squirtLevel,
    SIGNAL(valueChanged(int)), this, SLOT(squirtLevelRateSliderChanged(int)));
  
  QObject::connect(this->Internal->stillRenderSubsampleRate, 
    SIGNAL(valueChanged(int)), 
    this, SLOT(stillRenderSubsampleRateSliderChanged(int)));
  
  QObject::connect(this->Internal->clientCollect,
    SIGNAL(valueChanged(int)),
    this, SLOT(clientCollectSliderChanged(int)));
  
  
  // enable the apply button when things are changed
  QObject::connect(this->Internal->enableLOD,
                  SIGNAL(toggled(bool)),
                  this, SIGNAL(changesAvailable()));
  
  QObject::connect(this->Internal->immediateModeRendering,
                  SIGNAL(toggled(bool)),
                  this, SIGNAL(changesAvailable()));
  
  QObject::connect(this->Internal->triangleStrips,
                  SIGNAL(toggled(bool)),
                  this, SIGNAL(changesAvailable()));
  
  QObject::connect(this->Internal->renderingInterrupts,
                  SIGNAL(toggled(bool)),
                  this, SIGNAL(changesAvailable()));

  QObject::connect(this->Internal->enableCompositing,
                  SIGNAL(toggled(bool)),
                  this, SIGNAL(changesAvailable()));
  
  QObject::connect(this->Internal->orderedCompositing,
                  SIGNAL(toggled(bool)),
                  this, SIGNAL(changesAvailable()));

  QObject::connect(this->Internal->enableSubsampling,
                  SIGNAL(toggled(bool)),
                  this, SIGNAL(changesAvailable()));

  QObject::connect(this->Internal->enableSquirt,
                  SIGNAL(toggled(bool)),
                  this, SIGNAL(changesAvailable()));

  QObject::connect(this->Internal->enableStillRenderSubsampleRate,
                  SIGNAL(toggled(bool)),
                  this, SIGNAL(changesAvailable()));

  QObject::connect(this->Internal->enableClientCollect,
                  SIGNAL(toggled(bool)),
                  this, SIGNAL(changesAvailable()));

  for ( int cc = 0; cc < this->Internal->CameraControl3DComboBoxList.size(); cc++ )
    {
    QObject::connect(this->Internal->CameraControl3DComboBoxList[cc],
                    SIGNAL(currentIndexChanged(int)),
                    this, SIGNAL(changesAvailable()));
    }
  
  QObject::connect(this->Internal->resetCameraDefault,
    SIGNAL(clicked()), this, SLOT(resetDefaultCameraManipulators()));
  
}

void pqGlobalRenderViewOptions::setPage(const QString &page)
{
  if(page == "Render View")
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
QStringList pqGlobalRenderViewOptions::getPageList()
{
  QStringList pages("Render View");
  int count = this->Internal->stackedWidget->count();
  for(int i=0; i<count; i++)
    {
    pages << "Render View." + this->Internal->stackedWidget->widget(i)->objectName();
    }
  return pages;
}
 
//-----------------------------------------------------------------------------
void pqGlobalRenderViewOptions::applyChanges()
{
  pqSettings* settings = pqApplicationCore::instance()->settings();

  settings->beginGroup("renderModule");
  
  if (this->Internal->enableLOD->isChecked())
    {
    settings->setValue("LODThreshold", this->Internal->lodThreshold->value() / 10.0);
    settings->setValue("LODResolution", 160-this->Internal->lodResolution->value() + 10);
    }
  else
    {
    settings->setValue("LODThreshold", VTK_DOUBLE_MAX);
    }

  pqPipelineRepresentation::setUnstructuredGridOutlineThreshold(
    this->Internal->outlineThreshold->value()/10.0);

  settings->setValue("UseImmediateMode",
    this->Internal->immediateModeRendering->isChecked());
  
  settings->setValue("UseTriangleStrips",
    this->Internal->triangleStrips->isChecked());
  
  settings->setValue("RenderInterruptsEnabled",
    this->Internal->renderingInterrupts->isChecked());
  
  
  if(this->Internal->enableCompositing->isChecked())
    {
    settings->setValue("RemoteRenderThreshold", 
      this->Internal->compositeThreshold->value() / 10.0);
    }
  else
    {
    settings->setValue("RemoteRenderThreshold", VTK_DOUBLE_MAX);
    }

  settings->setValue("DisableOrderedCompositing",
        this->Internal->orderedCompositing->isChecked());


  if(this->Internal->enableSubsampling->isChecked())
    {
    settings->setValue("ImageReductionFactor", this->Internal->subsamplingRate->value());
    }
  else
    {
    settings->setValue("ImageReductionFactor", 1);
    }

  if (this->Internal->enableSquirt->isChecked())
    {
    settings->setValue("SquirtLevel", this->Internal->squirtLevel->value());
    }
  else
    {
    settings->setValue("SquirtLevel", 0);
    }

  
  if (this->Internal->enableStillRenderSubsampleRate->checkState() == Qt::Checked)
    {
    settings->setValue("StillRenderImageReductionFactor",
        this->Internal->stillRenderSubsampleRate->value());
    }
  else
    {
    settings->setValue("StillRenderImageReductionFactor", 1);
    }

  if (this->Internal->enableClientCollect->checkState() == Qt::Checked)
    {
    settings->setValue("CollectGeometryThreshold",
      this->Internal->clientCollect->value());
    }
  else
    {
    settings->setValue("CollectGeometryThreshold", VTK_DOUBLE_MAX);
    }
  
  // save out camera manipulators
  Manip manips[9];
  for(int i=0; i<9; i++)
    {
    manips[i] = DefaultManips[i];
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


  // loop through render views and apply new settings
  QList<pqRenderView*> views =
    pqApplicationCore::instance()->getServerManagerModel()->
    findItems<pqRenderView*>();

  foreach(pqRenderView* view, views)
    {
    view->restoreSettings(true);
    }

}

//-----------------------------------------------------------------------------
void pqGlobalRenderViewOptions::resetChanges()
{
  pqSettings* settings = pqApplicationCore::instance()->settings();

  settings->beginGroup("renderModule");

  QVariant val = settings->value("LODThreshold", 5);
  if(val.toDouble() >= VTK_LARGE_FLOAT)
    {
    this->Internal->enableLOD->setCheckState(Qt::Unchecked);
    this->Internal->updateLODThresholdLabel(this->Internal->lodThreshold->value());
    }
  else
    {
    this->Internal->enableLOD->setCheckState(Qt::Checked);
    this->Internal->lodThreshold->setValue(static_cast<int>(val.toDouble()*10));
    this->Internal->updateLODThresholdLabel(this->Internal->lodThreshold->value());
    }


  this->Internal->outlineThreshold->setValue(
    static_cast<int>(
      pqPipelineRepresentation::getUnstructuredGridOutlineThreshold()*10));
  this->Internal->updateOutlineThresholdLabel(this->Internal->outlineThreshold->value());
  
  val = settings->value("LODResolution", 50);
  this->Internal->lodResolution->setValue(static_cast<int>(160-val.toDouble() + 10));
  this->Internal->updateLODResolutionLabel(this->Internal->lodResolution->value());
  
  val = settings->value("UseImmediateMode", true);
  this->Internal->immediateModeRendering->setChecked(val.toBool());
  
  val = settings->value("UseTriangleStrips", false);
  this->Internal->triangleStrips->setChecked(val.toBool());
  
  val = settings->value("RenderInterruptsEnabled", false);
  this->Internal->renderingInterrupts->setChecked(val.toBool());

  
  //SquirtLevel"), 3);
  val = settings->value("RemoteRenderThreshold", 3);
  if(val.toDouble() >= VTK_LARGE_FLOAT)
    {
    this->Internal->enableCompositing->setCheckState(Qt::Unchecked);
    this->Internal->updateCompositeThresholdLabel(this->Internal->compositeThreshold->value());
    }
  else
    {
    this->Internal->enableCompositing->setCheckState(Qt::Checked);
    this->Internal->compositeThreshold->setValue(static_cast<int>(val.toDouble()*10));
    this->Internal->updateCompositeThresholdLabel(this->Internal->compositeThreshold->value());
    }
  
  
  val = settings->value("DisableOrderedCompositing", false);
  this->Internal->orderedCompositing->setChecked(val.toBool());

  val = settings->value("ImageReductionFactor", 2);
  if(val == 1)
    {
    this->Internal->enableSubsampling->setCheckState(Qt::Unchecked);
    this->Internal->updateSubsamplingRateLabel(this->Internal->subsamplingRate->value());
    }
  else
    {
    this->Internal->enableSubsampling->setCheckState(Qt::Checked);
    this->Internal->subsamplingRate->setValue(val.toInt());
    this->Internal->updateSubsamplingRateLabel(this->Internal->subsamplingRate->value());
    }

  val = settings->value("SquirtLevel", 3);
  if (val.toInt() == 0)
    {
    this->Internal->enableSquirt->setCheckState(Qt::Unchecked);
    this->Internal->updateSquirtLevelLabel(this->Internal->squirtLevel->value());
    }
  else
    {
    this->Internal->enableSquirt->setCheckState(Qt::Checked);
    this->Internal->squirtLevel->setValue(val.toInt());
    this->Internal->updateSquirtLevelLabel(this->Internal->squirtLevel->value());
    }

  val = settings->value("StillRenderImageReductionFactor", 1);
  
  if (val.toInt() == 1)
    {
    this->Internal->enableStillRenderSubsampleRate->setCheckState(Qt::Unchecked);
    this->Internal->updateStillSubsampleRateLabel(
      this->Internal->stillRenderSubsampleRate->value());
    }
  else
    {
    this->Internal->enableStillRenderSubsampleRate->setCheckState(Qt::Checked);
    this->Internal->stillRenderSubsampleRate->setValue(val.toInt());
    this->Internal->updateStillSubsampleRateLabel(
      this->Internal->stillRenderSubsampleRate->value());
    }
  
  val = settings->value("CollectGeometryThreshold", 100);
  if (val.toDouble() >= VTK_LARGE_FLOAT)
    {
    this->Internal->enableClientCollect->setCheckState(Qt::Unchecked);
    this->Internal->updateClientCollectLabel(this->Internal->clientCollect->value());
    }
  else
    {
    this->Internal->enableClientCollect->setCheckState(Qt::Checked);
    this->Internal->clientCollect->setValue(val.toInt());
    this->Internal->updateClientCollectLabel(this->Internal->clientCollect->value());
    }

  val = settings->value("InteractorStyle/CameraManipulators");

  Manip manips[9];
  for(int k = 0; k < 9; k++)
    {
    manips[k] = DefaultManips[k];
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
  
}


//-----------------------------------------------------------------------------
void pqGlobalRenderViewOptions::lodThresholdSliderChanged(int value)
{
  this->Internal->updateLODThresholdLabel(value);
  emit this->changesAvailable();
}

//-----------------------------------------------------------------------------
void pqGlobalRenderViewOptions::lodResolutionSliderChanged(int value)
{
  this->Internal->updateLODResolutionLabel(value);
  emit this->changesAvailable();
}
//-----------------------------------------------------------------------------
void pqGlobalRenderViewOptions::outlineThresholdSliderChanged(int value)
{
  this->Internal->updateOutlineThresholdLabel(value);
  emit this->changesAvailable();
}

//-----------------------------------------------------------------------------
void pqGlobalRenderViewOptions::compositeThresholdSliderChanged(int value)
{
  this->Internal->updateCompositeThresholdLabel(value);
  emit this->changesAvailable();
}

//-----------------------------------------------------------------------------
void pqGlobalRenderViewOptions::subsamplingRateSliderChanged(int value)
{
  this->Internal->updateSubsamplingRateLabel(value);
  emit this->changesAvailable();
}

//-----------------------------------------------------------------------------
void pqGlobalRenderViewOptions::squirtLevelRateSliderChanged(int value)
{
  this->Internal->updateSquirtLevelLabel(value);
  emit this->changesAvailable();
}

//-----------------------------------------------------------------------------
void pqGlobalRenderViewOptions::stillRenderSubsampleRateSliderChanged(int value)
{
  this->Internal->updateStillSubsampleRateLabel(value);
  emit this->changesAvailable();
}

//-----------------------------------------------------------------------------
void pqGlobalRenderViewOptions::clientCollectSliderChanged(int value)
{
  this->Internal->updateClientCollectLabel(static_cast<double>(value));
  emit this->changesAvailable();
}

//-----------------------------------------------------------------------------
void pqGlobalRenderViewOptions::resetDefaultCameraManipulators()
{
  for(int i=0; i<9; i++)
    {
    int idx = this->Internal->CameraControl3DComboItemList.indexOf(DefaultManips[i].Name);
    this->Internal->CameraControl3DComboBoxList[i]->setCurrentIndex(idx);
    }
}

