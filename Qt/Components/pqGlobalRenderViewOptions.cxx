/*=========================================================================

   Program: ParaView
   Module:    pqGlobalRenderViewOptions.cxx

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

/// \file pqGlobalRenderViewOptions.cxx
/// \date 7/20/2007

#include "pqGlobalRenderViewOptions.h"
#include "ui_pqGlobalRenderViewOptions.h"

#include <QPointer>
#include <QDoubleValidator>

#include "vtkType.h"
#include <vtksys/ios/sstream>

#include "pqApplicationCore.h"
#include "pqImageCompressorType.h"
#include "pqPipelineRepresentation.h"
#include "pqPluginManager.h"
#include "pqRenderView.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "pqSettings.h"
#include "pqViewModuleInterface.h"

typedef pqRenderView::ManipulatorType Manip;

class pqGlobalRenderViewOptions::pqInternal 
  : public Ui::pqGlobalRenderViewOptions
{
public:
  QList<QComboBox*> CameraControl3DComboBoxList;
  QList<QString> CameraControl3DComboItemList;
  QList<QComboBox*> CameraControl2DComboBoxList;
  QList<QString> CameraControl2DComboItemList;

public:
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

  void updateSquirtColorspaceLabel(int val)
    {
    static int bitValues[] = {24, 22, 19, 16, 13, 10};
    val = (val < 0 )? 0 : val;
    val = (val > 5) ? 5 : val;
    this->squirtColorspaceLabel->setText(
      QVariant(bitValues[val]).toString() + " Bits");
    }

  void updateZlibColorspaceLabel(int val)
    {
    static int bitValues[] = {24, 21, 18, 15, 12, 9};
    val = (val < 0) ? 0 : val;
    val = (val > 5) ? 5 : val;
    this->zlibColorspaceLabel->setText(
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
    << "Pan" << "Roll" << "Rotate" << "Multi-Rotate" << "Zoom";
  
  for ( int cc = 0; cc < this->Internal->CameraControl3DComboBoxList.size(); cc++ )
    {
    foreach(QString name, this->Internal->CameraControl3DComboItemList)
      {
      this->Internal->CameraControl3DComboBoxList.at(cc)->addItem(name);
      }
    }

  this->Internal->CameraControl2DComboBoxList << this->Internal->comboBoxCamera2D
      << this->Internal->comboBoxCamera2D_2 << this->Internal->comboBoxCamera2D_3
      << this->Internal->comboBoxCamera2D_4 << this->Internal->comboBoxCamera2D_5
      << this->Internal->comboBoxCamera2D_6 << this->Internal->comboBoxCamera2D_7
      << this->Internal->comboBoxCamera2D_8 << this->Internal->comboBoxCamera2D_9;

  this->Internal->CameraControl2DComboItemList //<< "FlyIn" << "FlyOut" << "Move"
     << "Pan" << "Zoom";

  for ( int cc = 0; cc < this->Internal->CameraControl2DComboBoxList.size(); cc++ )
    {
    foreach(QString name, this->Internal->CameraControl2DComboItemList)
      {
      this->Internal->CameraControl2DComboBoxList.at(cc)->addItem(name);
      }
    }

  QDoubleValidator* dv = new QDoubleValidator(this);
  this->Internal->offsetFactor->setValidator(dv);
  this->Internal->offsetUnits->setValidator(dv);
  this->Internal->zshift->setValidator(dv);
  this->Internal->restartWarningLabel->hide();

  // start fresh
  this->resetChanges();

  QObject::connect(this->Internal->lodThreshold,
    SIGNAL(valueChanged(int)), this, SLOT(lodThresholdSliderChanged(int)));

  QObject::connect(this->Internal->lodResolution,
    SIGNAL(valueChanged(int)), this, SLOT(lodResolutionSliderChanged(int)));

  QObject::connect(this->Internal->useOutlineForLOD,
                  SIGNAL(toggled(bool)),
                  this, SIGNAL(changesAvailable()));

  QObject::connect(this->Internal->outlineThreshold,
    SIGNAL(valueChanged(int)), this, SLOT(outlineThresholdSliderChanged(int)));

  QObject::connect(this->Internal->compositeThreshold,
    SIGNAL(valueChanged(int)), this, SLOT(compositeThresholdSliderChanged(int)));

  QObject::connect(this->Internal->subsamplingRate,
    SIGNAL(valueChanged(int)), this, SLOT(subsamplingRateSliderChanged(int)));

  QObject::connect(this->Internal->squirtColorspaceSlider,
    SIGNAL(valueChanged(int)), this, SLOT(squirtColorspaceSliderChanged(int)));

  QObject::connect(this->Internal->zlibColorspaceSlider,
    SIGNAL(valueChanged(int)), this, SLOT(zlibColorspaceSliderChanged(int)));

  QObject::connect(this->Internal->CompressorBWOpt,
    SIGNAL(currentIndexChanged(int)),this,SLOT(applyCompressorDefaults()));

  QObject::connect(this->Internal->stillRenderSubsampleRate, 
    SIGNAL(valueChanged(int)), 
    this, SLOT(stillRenderSubsampleRateSliderChanged(int)));

  // enable the apply button when things are changed
  QObject::connect(this->Internal->enableLOD,
                  SIGNAL(toggled(bool)),
                  this, SIGNAL(changesAvailable()));
  
  QObject::connect(this->Internal->immediateModeRendering,
                  SIGNAL(toggled(bool)),
                  this, SIGNAL(changesAvailable()));
  
  QObject::connect(this->Internal->depthPeeling,
                  SIGNAL(toggled(bool)),
                  this, SIGNAL(changesAvailable()));

  QObject::connect(this->Internal->numberOfPeels,
                  SIGNAL(valueChanged(int)),
                  this, SIGNAL(changesAvailable()));

  QObject::connect(this->Internal->useOffscreenRenderingForScreenshots,
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

  // Compressor
  QObject::connect(this->Internal->squirtEnable,
                  SIGNAL(toggled(bool)),
                  this, SIGNAL(changesAvailable()));

  QObject::connect(this->Internal->zlibEnable,
                  SIGNAL(toggled(bool)),
                  this, SIGNAL(changesAvailable()));

  QObject::connect(this->Internal->zlibCompressionLevel,
                  SIGNAL(valueChanged(int)),
                  this, SIGNAL(changesAvailable()));

  QObject::connect(this->Internal->zlibStripAlpha,
                  SIGNAL(toggled(bool)),
                  this, SIGNAL(changesAvailable()));

  QObject::connect(this->Internal->CompressorGroup,
                  SIGNAL(toggled(bool)),
                  this, SIGNAL(changesAvailable()));

  QObject::connect(this->Internal->enableStillRenderSubsampleRate,
                  SIGNAL(toggled(bool)),
                  this, SIGNAL(changesAvailable()));

  QObject::connect(this->Internal->stillRenderDelay,
                  SIGNAL(valueChanged(int)),
                  this, SIGNAL(changesAvailable()));

  for ( int cc = 0; cc < this->Internal->CameraControl3DComboBoxList.size(); cc++ )
    {
    QObject::connect(this->Internal->CameraControl3DComboBoxList[cc],
                    SIGNAL(currentIndexChanged(int)),
                    this, SIGNAL(changesAvailable()));
    }

  for ( int cc = 0; cc < this->Internal->CameraControl2DComboBoxList.size(); cc++ )
    {
    QObject::connect(this->Internal->CameraControl2DComboBoxList[cc],
                    SIGNAL(currentIndexChanged(int)),
                    this, SIGNAL(changesAvailable()));
    }

  QObject::connect(this->Internal->resetCameraDefault,
    SIGNAL(clicked()), this, SLOT(resetDefaultCameraManipulators()));

  QObject::connect(this->Internal->topologyMode,
    SIGNAL(currentIndexChanged(int)), this, SIGNAL(changesAvailable()));
  QObject::connect(this->Internal->offsetFactor,
    SIGNAL(textChanged(const QString&)), this, SIGNAL(changesAvailable()));
  QObject::connect(this->Internal->offsetUnits,
    SIGNAL(textChanged(const QString&)), this, SIGNAL(changesAvailable()));
  QObject::connect(this->Internal->zshift,
    SIGNAL(textChanged(const QString&)), this, SIGNAL(changesAvailable()));
  QObject::connect(this->Internal->offsetFaces,
    SIGNAL(toggled(bool)), this, SIGNAL(changesAvailable()));

  QObject::connect(this->Internal->topologyMode,
    SIGNAL(currentIndexChanged(int)),
    this->Internal->restartWarningLabel, SLOT(show()));
  QObject::connect(this->Internal->offsetFactor,
    SIGNAL(textChanged(const QString&)),
    this->Internal->restartWarningLabel, SLOT(show()));
  QObject::connect(this->Internal->offsetUnits,
    SIGNAL(textChanged(const QString&)),
    this->Internal->restartWarningLabel, SLOT(show()));
  QObject::connect(this->Internal->zshift,
    SIGNAL(textChanged(const QString&)),
    this->Internal->restartWarningLabel, SLOT(show()));
  QObject::connect(this->Internal->offsetFaces,
    SIGNAL(toggled(bool)),
    this->Internal->restartWarningLabel, SLOT(show()));

#if defined(__APPLE__)
  // Offscreen rendering is not needed on Mac (and it doesn't work on some
  // cards.
  this->Internal->useOffscreenRenderingForScreenshots->setEnabled(false);
  this->Internal->useOffscreenRenderingForScreenshots->setChecked(false);
#endif
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

  // This needs to be out of beginGroup()/endGroup().
  pqPipelineRepresentation::setUnstructuredGridOutlineThreshold(
    this->Internal->outlineThreshold->value()/10.0);

  pqServer::setGlobalImmediateModeRenderingSetting(
    this->Internal->immediateModeRendering->isChecked());

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
  settings->setValue("UseOutlineForLODRendering",
    this->Internal->useOutlineForLOD->isChecked());
  
  settings->setValue("DepthPeeling",
    this->Internal->depthPeeling->isChecked());

  settings->setValue("MaximumNumberOfPeels",
    this->Internal->numberOfPeels->value());

  settings->setValue("UseOffscreenRenderingForScreenshots",
    this->Internal->useOffscreenRenderingForScreenshots->isChecked());

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

  // Compressor Settings
  settings->setValue("CompressionEnabled",(int)this->Internal->CompressorGroup->isChecked());
  if (this->Internal->squirtEnable->isChecked())
    {
    settings->setValue("CompressorType",COMPRESSOR_SQUIRT);
    // build a configuration string that can be passed directly to
    // the compressor.
    std::ostringstream os;
    os << "vtkSquirtCompressor 0 "
       << this->Internal->squirtColorspaceSlider->value();
    settings->setValue("CompressorConfig",os.str().c_str());
    }
  else
  if (this->Internal->zlibEnable->isChecked())
    {
    settings->setValue("CompressorType",COMPRESSOR_ZLIB);
    // build a configuration string that can be passed directly to
    // the compressor.
    std::ostringstream os;
    os << "vtkZlibImageCompressor 0 "
       << this->Internal->zlibCompressionLevel->value()
       << " "
       << this->Internal->zlibColorspaceSlider->value()
       << " "
       << (int)this->Internal->zlibStripAlpha->isChecked();
    settings->setValue("CompressorConfig",os.str().c_str());
    }
  // save the UI settings, some of these are not be passed to
  // the underlying objects each time but we want to preserve.
  settings->setValue("SquirtColorSpace",this->Internal->squirtColorspaceSlider->value());
  settings->setValue("ZlibCompressionLevel",this->Internal->zlibCompressionLevel->value());
  settings->setValue("ZlibColorSpace",this->Internal->zlibColorspaceSlider->value());
  settings->setValue("ZlibStripAlpha",this->Internal->zlibStripAlpha->isChecked());
  settings->setValue("NonInteractiveRenderDelay",
    this->Internal->stillRenderDelay->value());

  if (this->Internal->enableStillRenderSubsampleRate->checkState() == Qt::Checked)
    {
    settings->setValue("StillRenderImageReductionFactor",
        this->Internal->stillRenderSubsampleRate->value());
    }
  else
    {
    settings->setValue("StillRenderImageReductionFactor", 1);
    }

  // save out camera manipulators
  Manip manips[9];
  const Manip* default3DManips = pqRenderView::getDefault3DManipulatorTypes();
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

  // Now save out 2D camera manipulators (these are saved in a different group).
  settings->beginGroup("renderModule2D");
  const Manip* default2DManips = pqRenderView::getDefault2DManipulatorTypes();
  for(int i=0; i<9; i++)
    {
    manips[i] = default2DManips[i];
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

  // loop through render views and apply new settings
  QList<pqRenderViewBase*> views =
    pqApplicationCore::instance()->getServerManagerModel()->
    findItems<pqRenderViewBase*>();

  foreach(pqRenderViewBase* view, views)
    {
    view->restoreSettings(true);
    }

  pqServer::setCoincidentTopologyResolutionModeSetting(
    this->Internal->topologyMode->currentIndex());
  pqServer::setPolygonOffsetParametersSetting(
    this->Internal->offsetFactor->text().toDouble(),
    this->Internal->offsetUnits->text().toDouble());
  pqServer::setPolygonOffsetFacesSetting(
    this->Internal->offsetFaces->isChecked());
  pqServer::setZShiftSetting(
    this->Internal->zshift->text().toDouble());
}

//-----------------------------------------------------------------------------
void pqGlobalRenderViewOptions::resetChanges()
{
  pqSettings* settings = pqApplicationCore::instance()->settings();

  // This needs to be out of beginGroup()/endGroup().
  this->Internal->outlineThreshold->setValue(
    static_cast<int>(
      pqPipelineRepresentation::getUnstructuredGridOutlineThreshold()*10));
  this->Internal->updateOutlineThresholdLabel(this->Internal->outlineThreshold->value());

  this->Internal->immediateModeRendering->setChecked(
    pqServer::globalImmediateModeRenderingSetting());

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

  val = settings->value("LODResolution", 50);
  this->Internal->lodResolution->setValue(static_cast<int>(160-val.toDouble() + 10));
  this->Internal->updateLODResolutionLabel(this->Internal->lodResolution->value());

  val = settings->value("UseOutlineForLODRendering", false);
  this->Internal->useOutlineForLOD->setChecked(val.toBool());

  val = settings->value("DepthPeeling", true);
  this->Internal->depthPeeling->setChecked(val.toBool());

  val = settings->value("MaximumNumberOfPeels", 4);
  this->Internal->numberOfPeels->setMinimum(1);
  this->Internal->numberOfPeels->setMaximum(100);
  this->Internal->numberOfPeels->setStrictRange(true);
  this->Internal->numberOfPeels->setValue(val.toInt());

  val = settings->value("UseOffscreenRenderingForScreenshots", false);
  if (getenv("PV_NO_OFFSCREEN_SCREENSHOTS"))
    {
    val = false;
    }
  this->Internal->useOffscreenRenderingForScreenshots->setChecked(val.toBool());

  val = settings->value("RenderInterruptsEnabled", false);
  this->Internal->renderingInterrupts->setChecked(val.toBool());


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

  // Compressor
  val = settings->value("CompressorType",COMPRESSOR_SQUIRT);
  switch (val.toInt())
    {
    case COMPRESSOR_ZLIB:
      this->Internal->squirtEnable->setChecked(false);
      this->Internal->zlibEnable->setChecked(true);
      break;
    case COMPRESSOR_SQUIRT:
    default:
      this->Internal->squirtEnable->setChecked(true);
      this->Internal->zlibEnable->setChecked(false);
      break;
    }
  val = settings->value("SquirtColorSpace",3);
  this->Internal->squirtColorspaceSlider->setValue(val.toInt());
  this->Internal->updateSquirtColorspaceLabel(val.toInt());
  val = settings->value("ZlibCompressionLevel",1);
  this->Internal->zlibCompressionLevel->setValue(val.toInt());
  val = settings->value("ZlibColorSpace",0);
  this->Internal->zlibColorspaceSlider->setValue(val.toInt());
  this->Internal->updateZlibColorspaceLabel(val.toInt());
  val = settings->value("ZlibStripAlpha",0);
  this->Internal->zlibStripAlpha->setChecked(val.toInt());
  val = settings->value("CompressionEnabled",1);
  this->Internal->CompressorGroup->setChecked(val.toInt());

  val = settings->value("NonInteractiveRenderDelay", 0);
  this->Internal->stillRenderDelay->setValue(val.toInt());

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
  
  val = settings->value("InteractorStyle/CameraManipulators");

  Manip manips[9];
  const Manip* default3DManips = pqRenderView::getDefault3DManipulatorTypes();
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

  settings->beginGroup("renderModule2D");
  val = settings->value("InteractorStyle/CameraManipulators");

  const Manip* default2DManips = pqRenderView::getDefault2DManipulatorTypes();
  for(int k = 0; k < 9; k++)
    {
    manips[k] = default2DManips[k];
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
    int idx = this->Internal->CameraControl2DComboItemList.indexOf(manips[i].Name);
    this->Internal->CameraControl2DComboBoxList[i]->setCurrentIndex(idx);
    }
  settings->endGroup();

  // setup coincident topology resolution settings.
  this->Internal->topologyMode->setCurrentIndex(
    pqServer::coincidentTopologyResolutionModeSetting());
  double factor, units;
  pqServer::polygonOffsetParametersSetting(factor, units);
  this->Internal->offsetFactor->setText(QString::number(factor));
  this->Internal->offsetUnits->setText(QString::number(units));
  this->Internal->offsetFaces->setChecked(
    pqServer::polygonOffsetFacesSetting());
  this->Internal->zshift->setText(QString::number(pqServer::zShiftSetting()));
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
void pqGlobalRenderViewOptions::squirtColorspaceSliderChanged(int value)
{
  this->Internal->updateSquirtColorspaceLabel(value);
  emit this->changesAvailable();
}

//-----------------------------------------------------------------------------
void pqGlobalRenderViewOptions::zlibColorspaceSliderChanged(int value)
{
  this->Internal->updateZlibColorspaceLabel(value);
  emit this->changesAvailable();
}

//-----------------------------------------------------------------------------
void pqGlobalRenderViewOptions::applyCompressorDefaults()
{
  enum {
    CONSUMER_DSL=1,
    ETHERNET_1_MEG,
    ETHERNET_1_GIG,
    ETHERNET_10_GIG,
    SHARED_MEMORY
    };

  int bwOpt=this->Internal->CompressorBWOpt->currentIndex();
  if (bwOpt==0) return;
  switch (bwOpt)
    {
    case CONSUMER_DSL:
      this->Internal->zlibEnable->setChecked(true);
      this->Internal->squirtEnable->setChecked(false);
      this->Internal->squirtColorspaceSlider->setValue(5);
      this->Internal->updateSquirtColorspaceLabel(5);
      this->Internal->zlibCompressionLevel->setValue(9);
      this->Internal->zlibColorspaceSlider->setValue(3);
      this->Internal->updateZlibColorspaceLabel(3);
      this->Internal->zlibStripAlpha->setChecked(true);
      this->Internal->subsamplingRate->setValue(3);
      this->Internal->enableSubsampling->setChecked(true);
      this->Internal->CompressorGroup->setChecked(true);
      break;

    case ETHERNET_1_MEG:
      this->Internal->zlibEnable->setChecked(true);
      this->Internal->squirtEnable->setChecked(false);
      this->Internal->squirtColorspaceSlider->setValue(3);
      this->Internal->updateSquirtColorspaceLabel(3);
      this->Internal->zlibCompressionLevel->setValue(6);
      this->Internal->zlibColorspaceSlider->setValue(2);
      this->Internal->updateZlibColorspaceLabel(2);
      this->Internal->zlibStripAlpha->setChecked(false);
      this->Internal->subsamplingRate->setValue(2);
      this->Internal->enableSubsampling->setChecked(true);
      this->Internal->CompressorGroup->setChecked(true);
      break;

    case ETHERNET_1_GIG:
      this->Internal->zlibEnable->setChecked(true);
      this->Internal->squirtEnable->setChecked(false);
      this->Internal->squirtColorspaceSlider->setValue(3);
      this->Internal->updateSquirtColorspaceLabel(3);
      this->Internal->zlibCompressionLevel->setValue(1);
      this->Internal->zlibColorspaceSlider->setValue(0);
      this->Internal->updateZlibColorspaceLabel(0);
      this->Internal->zlibStripAlpha->setChecked(false);
      this->Internal->subsamplingRate->setValue(2);
      this->Internal->enableSubsampling->setChecked(true);
      this->Internal->CompressorGroup->setChecked(true);
      break;

    case ETHERNET_10_GIG:
      this->Internal->zlibEnable->setChecked(false);
      this->Internal->squirtEnable->setChecked(true);
      this->Internal->squirtColorspaceSlider->setValue(3);
      this->Internal->updateSquirtColorspaceLabel(3);
      this->Internal->zlibCompressionLevel->setValue(1);
      this->Internal->zlibColorspaceSlider->setValue(0);
      this->Internal->updateZlibColorspaceLabel(0);
      this->Internal->zlibStripAlpha->setChecked(false);
      this->Internal->subsamplingRate->setValue(2);
      this->Internal->enableSubsampling->setChecked(false);
      this->Internal->CompressorGroup->setChecked(true);
      break;

    case SHARED_MEMORY:
      this->Internal->zlibEnable->setChecked(false);
      this->Internal->squirtEnable->setChecked(true);
      this->Internal->squirtColorspaceSlider->setValue(3);
      this->Internal->updateSquirtColorspaceLabel(3);
      this->Internal->zlibCompressionLevel->setValue(1);
      this->Internal->zlibColorspaceSlider->setValue(0);
      this->Internal->updateZlibColorspaceLabel(0);
      this->Internal->zlibStripAlpha->setChecked(false);
      this->Internal->subsamplingRate->setValue(2);
      this->Internal->enableSubsampling->setChecked(false);
      this->Internal->CompressorGroup->setChecked(false);
      break;
    }
  emit this->changesAvailable();
}

//-----------------------------------------------------------------------------
void pqGlobalRenderViewOptions::stillRenderSubsampleRateSliderChanged(int value)
{
  this->Internal->updateStillSubsampleRateLabel(value);
  emit this->changesAvailable();
}

//-----------------------------------------------------------------------------
void pqGlobalRenderViewOptions::resetDefaultCameraManipulators()
{
  const Manip* default3DManips = pqRenderView::getDefault3DManipulatorTypes();
  for(int i=0; i<9; i++)
    {
    int idx = this->Internal->CameraControl3DComboItemList.indexOf(
      default3DManips[i].Name);
    this->Internal->CameraControl3DComboBoxList[i]->setCurrentIndex(idx);
    }

  const Manip* default2DManips = pqRenderView::getDefault2DManipulatorTypes();
  for(int i=0; i<9; i++)
    {
    int idx = this->Internal->CameraControl2DComboItemList.indexOf(
      default2DManips[i].Name);
    this->Internal->CameraControl2DComboBoxList[i]->setCurrentIndex(idx);
    }
}
