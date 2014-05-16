/*=========================================================================

  Program:   Visualization Toolkit
  Module:    pqTransferFunctionEditor.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME pqTransferFunctionEditor
// .SECTION Thanks
// <verbatim>
//
//  This file is part of the PointSprites plugin developed and contributed by
//
//  Copyright (c) CSCS - Swiss National Supercomputing Centre
//                EDF - Electricite de France
//
//  John Biddiscombe, Ugo Varetto (CSCS)
//  Stephane Ploix (EDF)
//
// </verbatim>

#include "pqTransferFunctionEditor.h"
#include "ui_pqTransferFunctionEditor.h"

#include <QDoubleValidator>
#include <QTimer>
#include <QChar>

#include <iostream>
using namespace std;

// XPM data for pixmaps.
static const char * black_xpm[] = { "40 20 1 1", "+    c #000000",
    "++++++++++++++++++++++++++++++++++++++++",
    "++++++++++++++++++++++++++++++++++++++++",
    "++++++++++++++++++++++++++++++++++++++++",
    "++++++++++++++++++++++++++++++++++++++++",
    "++++++++++++++++++++++++++++++++++++++++",
    "++++++++++++++++++++++++++++++++++++++++",
    "++++++++++++++++++++++++++++++++++++++++",
    "++++++++++++++++++++++++++++++++++++++++",
    "++++++++++++++++++++++++++++++++++++++++",
    "++++++++++++++++++++++++++++++++++++++++",
    "++++++++++++++++++++++++++++++++++++++++",
    "++++++++++++++++++++++++++++++++++++++++",
    "++++++++++++++++++++++++++++++++++++++++",
    "++++++++++++++++++++++++++++++++++++++++",
    "++++++++++++++++++++++++++++++++++++++++",
    "++++++++++++++++++++++++++++++++++++++++",
    "++++++++++++++++++++++++++++++++++++++++",
    "++++++++++++++++++++++++++++++++++++++++",
    "++++++++++++++++++++++++++++++++++++++++",
    "++++++++++++++++++++++++++++++++++++++++" };

static const char * ramp_xpm[] = { "40 20 2 1", ".    c #000000",
    "+    c #FFFFFF", "......................................++",
    "....................................++++",
    "..................................++++++",
    "................................++++++++",
    "..............................++++++++++",
    "............................++++++++++++",
    "..........................++++++++++++++",
    "........................++++++++++++++++",
    "......................++++++++++++++++++",
    "....................++++++++++++++++++++",
    "..................++++++++++++++++++++++",
    "................++++++++++++++++++++++++",
    "..............++++++++++++++++++++++++++",
    "............++++++++++++++++++++++++++++",
    "..........++++++++++++++++++++++++++++++",
    "........++++++++++++++++++++++++++++++++",
    "......++++++++++++++++++++++++++++++++++",
    "....++++++++++++++++++++++++++++++++++++",
    "..++++++++++++++++++++++++++++++++++++++",
    "++++++++++++++++++++++++++++++++++++++++" };

static const char * inverse_ramp_xpm[] = { "40 20 2 1", ".    c #000000",
    "+    c #FFFFFF", "........................................",
    "++......................................",
    "++++....................................",
    "++++++..................................",
    "++++++++................................",
    "++++++++++..............................",
    "++++++++++++............................",
    "++++++++++++++..........................",
    "++++++++++++++++........................",
    "++++++++++++++++++......................",
    "++++++++++++++++++++....................",
    "++++++++++++++++++++++..................",
    "++++++++++++++++++++++++................",
    "++++++++++++++++++++++++++..............",
    "++++++++++++++++++++++++++++............",
    "++++++++++++++++++++++++++++++..........",
    "++++++++++++++++++++++++++++++++........",
    "++++++++++++++++++++++++++++++++++......",
    "++++++++++++++++++++++++++++++++++++....",
    "++++++++++++++++++++++++++++++++++++++.." };

static const char * white_xpm[] = { "40 20 1 1", "+    c #FFFFFF",
    "++++++++++++++++++++++++++++++++++++++++",
    "++++++++++++++++++++++++++++++++++++++++",
    "++++++++++++++++++++++++++++++++++++++++",
    "++++++++++++++++++++++++++++++++++++++++",
    "++++++++++++++++++++++++++++++++++++++++",
    "++++++++++++++++++++++++++++++++++++++++",
    "++++++++++++++++++++++++++++++++++++++++",
    "++++++++++++++++++++++++++++++++++++++++",
    "++++++++++++++++++++++++++++++++++++++++",
    "++++++++++++++++++++++++++++++++++++++++",
    "++++++++++++++++++++++++++++++++++++++++",
    "++++++++++++++++++++++++++++++++++++++++",
    "++++++++++++++++++++++++++++++++++++++++",
    "++++++++++++++++++++++++++++++++++++++++",
    "++++++++++++++++++++++++++++++++++++++++",
    "++++++++++++++++++++++++++++++++++++++++",
    "++++++++++++++++++++++++++++++++++++++++",
    "++++++++++++++++++++++++++++++++++++++++",
    "++++++++++++++++++++++++++++++++++++++++",
    "++++++++++++++++++++++++++++++++++++++++" };

// Qvis includes
#include <QvisGaussianOpacityBar.h>

// Paraview includes
#include "pqPipelineRepresentation.h"
#include "pqPropertyLinks.h"
#include "pqSMAdaptor.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkSMPVRepresentationProxy.h"
#include "vtkSMPointSpriteRepresentationProxy.h"
#include "vtkSMProperty.h"
#include "vtkSMProxy.h"
#include "vtkSMStringVectorProperty.h"

// vtk includes
#include "vtkSmartPointer.h"
#include "vtkEventQtSlotConnect.h"

#include "pqUndoStack.h"

class pqTransferFunctionEditor::pqInternals: public Ui::pqTransferFunctionEditor
{
public:
  pqPipelineRepresentation* Representation;
  vtkSmartPointer<vtkEventQtSlotConnect> VTKConnect;
  pqPropertyLinks Links;
  int BlockSignals;
  int Updating;

  const char* UseScalarRangeProperty;
  const char* ArrayNameProperty;
  const char* TransferFunctionModeProperty;
  const char* TableValuesProperty;
  const char* RangeProperty;
  const char* ScalarRangeProperty;
  const char* GaussianControlPointsProperty;
  const char* ConstantNameProperty;
  const char* ArrayComponentProperty;
  const char* ProportionalFactor;
  const char* IsProportional;

  pqInternals()
  {
    Representation = NULL;
    VTKConnect = vtkSmartPointer<vtkEventQtSlotConnect>::New();
    BlockSignals = 0;
    Updating = 0;
  }

};

pqTransferFunctionEditor::pqTransferFunctionEditor()
{
  this->Internals = new pqTransferFunctionEditor::pqInternals();
  this->Internals->setupUi(this);

  //this->initialize();

  // We are usinging Queued slot execution where ever possible,
  // This ensures that the updateAllViews() slot is called
  // only after the vtkSMProperty has been changed by the pqPropertyLinks.

  //
  // Buttons for scribble editor
  //
  QPixmap blackPixmap(black_xpm);
  QPixmap rampPixmap(ramp_xpm);
  QPixmap iRampPixmap(inverse_ramp_xpm);
  QPixmap whitePixmap(white_xpm);
  this->Internals->zeroButton->setIcon(blackPixmap);
  this->Internals->rampButton->setIcon(rampPixmap);
  this->Internals->iRampButton->setIcon(iRampPixmap);
  this->Internals->oneButton->setIcon(whitePixmap);

  connect(this->Internals->zeroButton, SIGNAL(clicked()),
      this->Internals->scribbleEditor, SLOT(makeTotallyZero()),
      Qt::QueuedConnection);
  connect(this->Internals->rampButton, SIGNAL(clicked()),
      this->Internals->scribbleEditor, SLOT(makeLinearRamp()),
      Qt::QueuedConnection);
  connect(this->Internals->iRampButton, SIGNAL(clicked()),
      this->Internals->scribbleEditor, SLOT(makeInverseLinearRamp()),
      Qt::QueuedConnection);
  connect(this->Internals->oneButton, SIGNAL(clicked()),
      this->Internals->scribbleEditor, SLOT(makeTotallyOne()),
      Qt::QueuedConnection);

  QButtonGroup* radiusGroup = new QButtonGroup(this);
  radiusGroup->addButton(this->Internals->freeform);
  radiusGroup->addButton(this->Internals->gaussian);

  //
  // Interaction mode
  //
  connect(this->Internals->freeform, SIGNAL(toggled(bool)), this,
      SLOT(onFreeFormToggled(bool)));

  //
  // Connect the opacity editor widgets
  //
  this->Internals->editorStack->setCurrentWidget(
      this->Internals->scribbleEditor);

  connect(this->Internals->gaussianEditor, SIGNAL(mouseReleased()), this,
      SLOT(onGaussianValuesModified()), Qt::QueuedConnection);
  connect(this->Internals->scribbleEditor, SIGNAL(opacitiesChanged()), this,
      SLOT(onTableValuesModified()), Qt::QueuedConnection);
  connect(this->Internals->scribbleEditor, SIGNAL(mouseReleased()), this,
      SLOT(onTableValuesModified()), Qt::QueuedConnection);

  QDoubleValidator* validator;
  validator = new QDoubleValidator(this->Internals->scaleMin);
  validator->setBottom(0.0);
  this->Internals->scaleMin->setValidator(validator);

  validator = new QDoubleValidator(this->Internals->scaleMax);
  validator->setBottom(0.0);
  this->Internals->scaleMax->setValidator(validator);

  validator = new QDoubleValidator(this->Internals->scalarMin);
  this->Internals->scalarMin->setValidator(validator);

  validator = new QDoubleValidator(this->Internals->scalarMax);
  this->Internals->scalarMax->setValidator(validator);

  validator = new QDoubleValidator(this->Internals->propEdit);
  validator->setBottom(0.0);
  this->Internals->propEdit->setValidator(validator);

  this->connect(this->Internals->useScalarRange, SIGNAL(toggled(bool)), this,
      SLOT(onAutoScalarRange(bool)));

  this->connect(this->Internals->scaleMin, SIGNAL(valueChanged(double)), this,
      SLOT(onScaleRangeModified()), Qt::QueuedConnection);

  this->connect(this->Internals->scaleMax, SIGNAL(valueChanged(double)), this,
      SLOT(onScaleRangeModified()), Qt::QueuedConnection);

  this->connect(this->Internals->scalarMin, SIGNAL(valueChanged(double)), this,
      SLOT(onScalarRangeModified()), Qt::QueuedConnection);

  this->connect(this->Internals->scalarMax, SIGNAL(valueChanged(double)), this,
      SLOT(onScalarRangeModified()), Qt::QueuedConnection);

  this->connect(this->Internals->propBox, SIGNAL(toggled(bool)), this,
      SLOT(onProportionnalToggled(bool)), Qt::QueuedConnection);

  this->connect(this->Internals->propEdit, SIGNAL(valueChanged(double)), this,
      SLOT(onProportionnalEdited()), Qt::QueuedConnection);

}

pqTransferFunctionEditor::~pqTransferFunctionEditor()
{
  delete this->Internals;
  this->Internals = 0;
}

void pqTransferFunctionEditor::needReloadGUI()
{
  if (this->Internals->Updating)
    {
    return;
    }
  this->Internals->Updating = true;
  QTimer::singleShot(0, this, SLOT(reloadGUI()));
}

void pqTransferFunctionEditor::reloadGUI()
{
  this->Internals->Updating = false;
  vtkSMProxy
      * reprProxy =
          (this->Internals->Representation) ? this->Internals->Representation->getProxy()
              : NULL;
  if (!reprProxy)
    return;

  this->Internals->BlockSignals++;

  // get all the values from the SM

  int useScalarRange = pqSMAdaptor::getElementProperty(reprProxy->GetProperty(
      this->Internals->UseScalarRangeProperty)).toInt();
  QString transfertFunctionMode =
      pqSMAdaptor::getEnumerationProperty(reprProxy->GetProperty(
          this->Internals->TransferFunctionModeProperty)).toString();
  QList<QVariant> tableValues = pqSMAdaptor::getMultipleElementProperty(
      reprProxy->GetProperty(this->Internals->TableValuesProperty));
  QList<QVariant> gaussianValues = pqSMAdaptor::getMultipleElementProperty(
      reprProxy->GetProperty(this->Internals->GaussianControlPointsProperty));
  QList<QVariant> scaleRange = pqSMAdaptor::getMultipleElementProperty(
      reprProxy->GetProperty(this->Internals->RangeProperty));
  QList<QVariant> scalarRange = pqSMAdaptor::getMultipleElementProperty(
      reprProxy->GetProperty(this->Internals->ScalarRangeProperty));

  this->Internals->freeform->setChecked(transfertFunctionMode == "Table");

  //bool setAutoScalarRange = false;
  if (useScalarRange == 0)
    {
    this->Internals->scalarMin->setValue(scalarRange[0].toDouble());
    this->Internals->scalarMax->setValue(scalarRange[1].toDouble());
    }
  else
    {
    onAutoScalarRange(true);
    }

  // the opacity editor has no scale range property, so the returned list is empty
  if (scaleRange.size() == 2)
    {
    this->Internals->scaleMin->setValue(scaleRange[0].toDouble());
    this->Internals->scaleMax->setValue(scaleRange[1].toDouble());
    }

  this->setGaussianControlPoints(gaussianValues);
  this->setFreeformValues(tableValues);

  this->Internals->BlockSignals--;
}

void pqTransferFunctionEditor::setRepresentation(pqPipelineRepresentation* repr)
{
  if (this->Internals->Representation == repr)
    {
    return;
    }

  this->Internals->Representation = repr;

  this->Internals->Links.removeAllPropertyLinks();
  this->Internals->VTKConnect->Disconnect();

  vtkSMProxy* reprProxy = (repr) ? repr->getProxy() : NULL;

  if (reprProxy)
    {
    vtkSMProperty* prop;

    prop = reprProxy->GetProperty(this->Internals->UseScalarRangeProperty);
    if (prop)
      {
      this->Internals->Links.addPropertyLink(this->Internals->useScalarRange,
          "checked", SIGNAL(toggled(bool)), reprProxy, prop);
      }

    prop = reprProxy->GetProperty(this->Internals->ProportionalFactor);
    if (prop)
      {
      this->Internals->Links.addPropertyLink(this->Internals->propEdit,
          "value", SIGNAL(valueChanged(double)), reprProxy, prop);
      }

    prop = reprProxy->GetProperty(this->Internals->IsProportional);
    if (prop)
      {
      this->Internals->Links.addPropertyLink(this->Internals->propBox,
          "checked", SIGNAL(toggled(bool)), reprProxy, prop);
      }

    prop = reprProxy->GetProperty("Representation");
    if (prop)
      {
      this->Internals->VTKConnect->Connect(prop, vtkCommand::ModifiedEvent,
          this, SLOT(needReloadGUI()), NULL, 0.0, Qt::QueuedConnection);
      }
    }

  needReloadGUI();
}

void pqTransferFunctionEditor::configure(EditorConfiguration conf)
{
  if (conf == Opacity)
    {
    this->Internals->titleStack->setCurrentWidget(this->Internals->noTitlePage);
    this->Internals->legendStack->setCurrentWidget(
        this->Internals->opacityLegend);

    this->Internals->ConstantNameProperty = "Constant Opacity";
    this->Internals->UseScalarRangeProperty = "OpacityUseScalarRange";
    this->Internals->ArrayNameProperty = "OpacityArray";
    this->Internals->ArrayComponentProperty = "OpacityVectorComponent";
    this->Internals->TransferFunctionModeProperty
        = "OpacityTransferFunctionMode";
    this->Internals->TableValuesProperty = "OpacityTableValues";
    this->Internals->RangeProperty = NULL;
    this->Internals->ScalarRangeProperty = "OpacityScalarRange";
    this->Internals->GaussianControlPointsProperty
        = "OpacityGaussianControlPoints";
    this->Internals->ProportionalFactor = "OpacityProportionalFactor";
    this->Internals->IsProportional = "OpacityIsProportional";
    }
  else if (conf == Radius)
    {
    this->Internals->titleStack->setCurrentWidget(this->Internals->scalePage);
    this->Internals->legendStack->setCurrentWidget(
        this->Internals->radiusLegend);

    this->Internals->ConstantNameProperty = "Constant Radius";
    this->Internals->UseScalarRangeProperty = "RadiusUseScalarRange";
    this->Internals->ArrayNameProperty = "RadiusArray";
    this->Internals->ArrayComponentProperty = "RadiusVectorComponent";
    this->Internals->TransferFunctionModeProperty
        = "RadiusTransferFunctionMode";
    this->Internals->TableValuesProperty = "RadiusTableValues";
    this->Internals->RangeProperty = "RadiusRange";
    this->Internals->ScalarRangeProperty = "RadiusScalarRange";
    this->Internals->GaussianControlPointsProperty
        = "RadiusGaussianControlPoints";
    this->Internals->ProportionalFactor = "RadiusProportionalFactor";
    this->Internals->IsProportional = "RadiusIsProportional";
    }
}

void pqTransferFunctionEditor::onFreeFormToggled(bool freeFormOn)
{
  pqPipelineRepresentation* repr = this->Internals->Representation;
  vtkSMProxy* reprProxy = (repr) ? repr->getProxy() : NULL;

  const char* mode = NULL;

  if (freeFormOn)
    {
    this->Internals->editorStack->setCurrentWidget(
        this->Internals->scribbleEditor);
    mode = "Table";
    }
  else
    {
    this->Internals->editorStack->setCurrentWidget(
        this->Internals->gaussianEditor);

    mode = "Gaussian";
    }

  if (!reprProxy)
    return;

  pqSMAdaptor::setEnumerationProperty(reprProxy->GetProperty(
      this->Internals->TransferFunctionModeProperty), mode);

  if (this->Internals->BlockSignals)
    return;

  reprProxy->UpdateVTKObjects();
  this->updateAllViews();
}

QList<QVariant> pqTransferFunctionEditor::freeformValues()
{
  int resolution = 256;
  float* opacities = new float[resolution];
  this->Internals->scribbleEditor->getRawOpacities(resolution, opacities);
  QList<QVariant> list;
  for (int i = 0; i < resolution; ++i)
    {
    list.append(QVariant(static_cast<double> (opacities[i])));
    }
  delete opacities;
  return list;
}

QList<QVariant> pqTransferFunctionEditor::gaussianControlPoints()
{
  QList<QVariant> list;
  for (int i = 0; i < this->Internals->gaussianEditor->getNumberOfGaussians(); ++i)
    {
    float g[5];
    this->Internals->gaussianEditor->getGaussian(i, &g[0], &g[1], &g[2], &g[3],
        &g[4]);

    for (int j = 0; j < 5; j++)
      {
      list.append(QVariant(static_cast<double> (g[j])));
      }
    }
  return list;
}

void pqTransferFunctionEditor::setFreeformValues(const QList<QVariant>& values)
{
  this->Internals->scribbleEditor->blockSignals(true);
  int n = values.size();
  if (n == 0)
    return;

  float* vals = new float[n];

  for (int i = 0; i < n; ++i)
    vals[i] = static_cast<float> (values[i].toDouble());

  this->Internals->scribbleEditor->setRawOpacities(n, vals);
  this->Internals->scribbleEditor->blockSignals(false);
  delete vals;
}

void pqTransferFunctionEditor::setGaussianControlPoints(const QList<QVariant>& values)
{
  this->Internals->gaussianEditor->blockSignals(true);
  //
  this->Internals->gaussianEditor->setAllGaussians(0, NULL);
  int n = values.size();
  if (n > 0)
    {
    float gcpts[1024];
    for (int i = 0; i < n; ++i)
      gcpts[i] = static_cast<float> (values[i].toDouble());
    // Set all of the gaussians into the widget.
    this->Internals->gaussianEditor->setAllGaussians(n / 5, gcpts);
    }
  this->Internals->gaussianEditor->blockSignals(false);
}

void pqTransferFunctionEditor::onProportionnalToggled(bool propOn)
{
  if (propOn)
    {
    this->Internals->scribbleEditor->makeLinearRamp();
    onFreeFormToggled(true);
    onProportionnalEdited();
    }
}

void pqTransferFunctionEditor::onProportionnalEdited()
{
  pqPipelineRepresentation* repr = this->Internals->Representation;
  vtkSMProxy* reprProxy = (repr) ? repr->getProxy() : NULL;
  if (!reprProxy)
    return;

  int isProportional = pqSMAdaptor::getElementProperty(reprProxy->GetProperty(
      this->Internals->IsProportional)).toInt();

  if(!isProportional)
    return;

  double ff = this->Internals->propEdit->value();

  this->Internals->scaleMin->setValue(this->Internals->scalarMin->value()
      * ff);
  this->Internals->scaleMax->setValue(this->Internals->scalarMax->value()
      * ff);

}

void pqTransferFunctionEditor::onGaussianValuesModified()
{
  pqPipelineRepresentation* repr = this->Internals->Representation;
  vtkSMProxy* reprProxy = (repr) ? repr->getProxy() : NULL;
  if (!reprProxy)
    return;

  this->SetProxyValue(this->Internals->GaussianControlPointsProperty,
      this->gaussianControlPoints(), false);

  if (this->Internals->BlockSignals)
    return;

  reprProxy->UpdateVTKObjects();
  this->updateAllViews();
}

void pqTransferFunctionEditor::onTableValuesModified()
{
  pqPipelineRepresentation* repr = this->Internals->Representation;
  vtkSMProxy* reprProxy = (repr) ? repr->getProxy() : NULL;
  if (!reprProxy)
    return;

  this->SetProxyValue(this->Internals->TableValuesProperty,
      this->freeformValues(), false);

  if (this->Internals->BlockSignals)
    return;

  reprProxy->UpdateVTKObjects();
  this->updateAllViews();
}

void pqTransferFunctionEditor::onScalarRangeModified()
{
  QList<QVariant> range;
  range.append(this->Internals->scalarMin->value());
  range.append(this->Internals->scalarMax->value());

  this->SetProxyValue(this->Internals->ScalarRangeProperty, range);

  pqPipelineRepresentation* repr = this->Internals->Representation;
  vtkSMProxy * reprProxy = (repr ? repr->getProxy() : NULL);
  if (!reprProxy)
    {
    return;
    }

  if (pqSMAdaptor::getElementProperty(reprProxy->GetProperty(
      this->Internals->IsProportional)).toInt() == 1)
    {
    onProportionnalEdited();
    }
}

void pqTransferFunctionEditor::onScaleRangeModified()
{
  QList<QVariant> range;
  range.append(this->Internals->scaleMin->value());
  range.append(this->Internals->scaleMax->value());

  this->SetProxyValue(this->Internals->RangeProperty, range);
}

void pqTransferFunctionEditor::onAutoScalarRange(bool autoRange)
{
  if (autoRange)
    {
    pqPipelineRepresentation* repr = this->Internals->Representation;
    vtkSMProxy * reprProxy = (repr ? repr->getProxy() : NULL);
    if (!reprProxy)
      {
      return;
      }

    const char* array =
        vtkSMStringVectorProperty::SafeDownCast(reprProxy->GetProperty(
            this->Internals->ArrayNameProperty))->GetElement(4);
    int comp = pqSMAdaptor::getElementProperty(reprProxy->GetProperty(
        this->Internals->ArrayComponentProperty)).toInt();

    if (strcmp(array, this->Internals->ConstantNameProperty) == 0 || strcmp(
        array, "") == 0)
      return;

    double drange[2] = {0.0, 1.0};
    if (vtkPVArrayInformation* arrayInfo =
      vtkSMPVRepresentationProxy::GetArrayInformationForColorArray(reprProxy))
      {
      arrayInfo->GetComponentRange(comp, drange);
      if (drange[1] < drange[0])
        {
        drange[0] = 0.0;
        drange[1] = 1.0;
        }
      }
    this->Internals->scalarMin->setValue(drange[0]);
    this->Internals->scalarMax->setValue(drange[1]);

    if (pqSMAdaptor::getElementProperty(reprProxy->GetProperty(
        this->Internals->IsProportional)).toInt() == 1)
      {
      onProportionnalEdited();
      }
    }
}

//----------------------------------------------------------------------------
// The routines below here should be in some generic proxy set/get class
//----------------------------------------------------------------------------
void pqTransferFunctionEditor::SetProxyValue(const char *name,
    QList<QVariant> val,
    bool updateFlag)
{
  vtkSMProxy
      *reprProxy =
          (this->Internals->Representation ? this->Internals->Representation->getProxy()
              : NULL);
  if (!reprProxy)
    {
    return;
    }

  vtkSMProperty* Property = reprProxy->GetProperty(name);
  pqSMAdaptor::setMultipleElementProperty(Property, val);
  if (updateFlag && !this->Internals->BlockSignals)
    {
    BEGIN_UNDO_EXCLUDE();
    reprProxy->UpdateVTKObjects();
    this->updateAllViews();
    END_UNDO_EXCLUDE();
    }
}
//----------------------------------------------------------------------------
QList<QVariant> pqTransferFunctionEditor::GetProxyValueList(const char *name)
{
  vtkSMProxy
      *proxy =
          (this->Internals->Representation ? this->Internals->Representation->getProxy()
              : NULL);
  if (!this->Internals->Representation || !proxy)
    {
    return QList<QVariant> ();
    }

  vtkSMProperty* Property = proxy->GetProperty(name);
  return pqSMAdaptor::getMultipleElementProperty(Property);
}

void pqTransferFunctionEditor::updateAllViews()
{
  if (this->Internals->Representation)
    {
    this->Internals->Representation->renderViewEventually();
    }
}

