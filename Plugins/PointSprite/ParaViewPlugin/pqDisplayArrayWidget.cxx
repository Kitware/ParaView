/*=========================================================================

  Program:   Visualization Toolkit
  Module:    pqDisplayArrayWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME pqDisplayArrayWidget
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

#include "pqDisplayArrayWidget.h"

#include "vtkDataObject.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkSMOutputPort.h"
#include "vtkSMProperty.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QIcon>
#include <QRegExp>

#include "pqApplicationCore.h"
#include "pqOutputPort.h"
#include "pqPipelineRepresentation.h"
#include "pqScalarsToColors.h"
#include "pqSMAdaptor.h"
#include "pqTimer.h"
#include "pqUndoStack.h"

class pqDisplayArrayWidget::pqInternal
{
public:
  pqInternal(QWidget* vtkNotUsed(parentObject))
  {
    this->CellDataIcon = new QIcon(
        ":/pqWidgets/Icons/pqCellData16.png");
    this->PointDataIcon = new QIcon(
        ":/pqWidgets/Icons/pqPointData16.png");
    this->SolidColorIcon = new QIcon(
        ":/pqWidgets/Icons/pqSolidColor16.png");
    this->VTKConnect = vtkEventQtSlotConnect::New();
    this->BlockEmission = 0;
    this->Updating = false;
  }
  ~pqInternal()
  {
  delete this->CellDataIcon;
  delete this->PointDataIcon;
  delete this->SolidColorIcon;
  this->VTKConnect->Delete();
  }

  QIcon* CellDataIcon;
  QIcon* PointDataIcon;
  QIcon* SolidColorIcon;

  QHBoxLayout* Layout;
  QComboBox* Variables;
  QComboBox* Components;
  int BlockEmission;
  bool Updating;
  vtkEventQtSlotConnect* VTKConnect;
  QPointer<pqPipelineRepresentation> Representation;

  QString PropertyArrayName;
  QString PropertyArrayComponent;

  QString ConstantVariableName;
  QString ToolTip;
};

//-----------------------------------------------------------------------------
pqDisplayArrayWidget::pqDisplayArrayWidget(QWidget *parentObject) : QWidget(parentObject)
{
  this->Internal = new pqInternal(this);

  this->Internal->Layout = new QHBoxLayout(this);
  this->Internal->Layout->setMargin(0);

  this->Internal->Variables = new QComboBox(this);
  this->Internal->Variables->setMaxVisibleItems(60);
  this->Internal->Variables->setObjectName("Variables");
  this->Internal->Variables->setMinimumSize(QSize(150, 0));
  this->Internal->Variables->setSizeAdjustPolicy(QComboBox::AdjustToContents);

  this->Internal->Components = new QComboBox(this);
  this->Internal->Components->setObjectName("Components");

  this->Internal->Layout->addWidget(this->Internal->Variables);
  this->Internal->Layout->addWidget(this->Internal->Components);

  this->Internal->ConstantVariableName = "Solid Color";

  this->Internal->PropertyArrayName = "";
  this->Internal->PropertyArrayComponent = "";
  this->Internal->ToolTip = "";

  QObject::connect(this->Internal->Variables, SIGNAL(currentIndexChanged(int)),
      SLOT(onVariableActivated(int)));
  QObject::connect(this->Internal->Components, SIGNAL(currentIndexChanged(int)),
      SLOT(onComponentActivated(int)));

}

//-----------------------------------------------------------------------------
pqDisplayArrayWidget::~pqDisplayArrayWidget()
{
  delete this->Internal;
}

void pqDisplayArrayWidget::setToolTip(const QString& tooltip)
{
  this->Internal->ToolTip = tooltip;
  this->Internal->Variables->setToolTip(tooltip);
  this->Internal->Components->setToolTip(tooltip);
}

//-----------------------------------------------------------------------------
QString pqDisplayArrayWidget::getCurrentText() const
{
  return this->Internal->Variables->currentText();
}

//-----------------------------------------------------------------------------
QString pqDisplayArrayWidget::currentVariableName() const
{
  QString txt = this->getCurrentText();
  if (txt != this->Internal->ConstantVariableName)
    {
    return txt;
    }
  return QString();
}

//-----------------------------------------------------------------------------
int pqDisplayArrayWidget::currentComponent() const
{
  if (this->Internal->Components->count() > 1)
    {
    return this->Internal->Components->currentIndex()-1;
    }
  return -1;
}

//-----------------------------------------------------------------------------
void pqDisplayArrayWidget::clear()
{
  this->Internal->BlockEmission++;
  this->Internal->Variables->clear();
  this->Internal->BlockEmission--;
}

//-----------------------------------------------------------------------------
void pqDisplayArrayWidget::onComponentActivated(int row)
{
  if (this->Internal->BlockEmission)
    {
    return;
    }

  if (row == 0)
    {
    emit this->componentChanged(pqScalarsToColors::MAGNITUDE, -1);
    }
  else
    {
    emit this->componentChanged(pqScalarsToColors::COMPONENT, row - 1);
    }
  emit this->modified();
}

//-----------------------------------------------------------------------------
void pqDisplayArrayWidget::onVariableActivated(int row)
{
  Q_UNUSED(row);

  if (this->Internal->BlockEmission)
    {
    return;
    }

  emit this->variableChanged(this->Internal->Variables->currentText());
  emit this->modified();
}

//-----------------------------------------------------------------------------
void pqDisplayArrayWidget::updateGUI()
{
  this->Internal->BlockEmission++;
  pqPipelineRepresentation* display = this->getRepresentation();
  if (display)
    {
    QString name = this->getArrayName();
    int index = this->Internal->Variables->findText(name);
    if (index < 0)
      {
      index = 0;
      }
    this->Internal->Variables->setCurrentIndex(index);
    }
  this->Internal->BlockEmission--;

  this->updateComponents();
}

//-----------------------------------------------------------------------------
void pqDisplayArrayWidget::updateComponents()
{
  this->Internal->BlockEmission++;
  pqPipelineRepresentation* display = this->getRepresentation();
  vtkSMProxy * repr = (display ? display->getProxy() : NULL);
  int comp = -1;
  if (display != NULL && repr != NULL)
    {
    comp = pqSMAdaptor::getElementProperty(repr->GetProperty(
        this->Internal->PropertyArrayComponent.toLatin1().data())).toInt();
    vtkPVArrayInformation* ai = this->getArrayInformation();
    int numComponents = ai? ai->GetNumberOfComponents() : 1;
    if (numComponents == 1 || comp >= numComponents)
      {
      comp = -1;
      }
    }
  this->Internal->Components->setCurrentIndex(comp + 1);
  this->Internal->BlockEmission--;
}

//-----------------------------------------------------------------------------
vtkPVArrayInformation* pqDisplayArrayWidget::getArrayInformation()
{
  pqPipelineRepresentation* display = this->getRepresentation();
  vtkSMProxy * repr = (display ? display->getProxy() : NULL);
  QString arrayName = this->getArrayName();
  if (repr != NULL &&
    arrayName.isEmpty() == false &&
    arrayName != this->Internal->ConstantVariableName)
    {
    vtkPVDataInformation* dataInfo = display->getInputDataInformation();
    vtkPVArrayInformation* ai = dataInfo->GetArrayInformation(
      arrayName.toLatin1().data(), vtkDataObject::POINT);
    return ai;
    }
  return NULL;
}

//-----------------------------------------------------------------------------
void pqDisplayArrayWidget::reloadComponents()
{
  this->Internal->BlockEmission++;
  this->Internal->Components->clear();

  pqPipelineRepresentation* display = this->getRepresentation();
  if (display)
    {
    vtkPVArrayInformation* ai = this->getArrayInformation();
    int numComponents = ai? ai->GetNumberOfComponents() : 1;
    if (numComponents > 1)
      {
      Q_ASSERT(ai);
      this->Internal->Components->addItem("Magnitude");
      QString componentName;
      for (int i = 0; i < numComponents; i++)
        {
        componentName =  ai->GetComponentName(i);
        this->Internal->Components->addItem( componentName );
        }
      }
    }
  this->Internal->BlockEmission--;
  this->updateComponents();
}

//-----------------------------------------------------------------------------
void pqDisplayArrayWidget::setRepresentation(pqPipelineRepresentation* display)
{
  if (display == this->Internal->Representation)
    {
    return;
    }

  if (this->Internal->Representation)
    {
    QObject::disconnect(this->Internal->Representation, 0, this, 0);
    }

  this->Internal->VTKConnect->Disconnect();
  this->Internal->Representation = qobject_cast<pqPipelineRepresentation*> (
      display);

  if (this->Internal->Representation)
    {
    vtkSMProxy* repr = this->Internal->Representation->getProxy();

    // if the domain has been modified, we need to reload the combo boxes
    if(repr->GetProperty(this->Internal->PropertyArrayName.toLatin1()) != NULL)
      {
      this->Internal->VTKConnect->Connect(repr->GetProperty(
        this->Internal->PropertyArrayName.toLatin1()),
        vtkCommand::DomainModifiedEvent, this, SLOT(needReloadGUI()), NULL, 0.0,
        Qt::QueuedConnection);

      this->Internal->VTKConnect->Connect(repr->GetProperty(
        this->Internal->PropertyArrayName.toLatin1()),
        vtkCommand::ModifiedEvent, this, SLOT(updateGUI()), NULL, 0.0,
        Qt::QueuedConnection);
      }

    if(repr->GetProperty(this->Internal->PropertyArrayComponent.toLatin1()) != NULL)
      {
      this->Internal->VTKConnect->Connect(repr->GetProperty(
        this->Internal->PropertyArrayComponent.toLatin1()),
        vtkCommand::DomainModifiedEvent, this, SLOT(needReloadGUI()), NULL, 0.0,
        Qt::QueuedConnection);

      this->Internal->VTKConnect->Connect(repr->GetProperty(
        this->Internal->PropertyArrayComponent.toLatin1()),
        vtkCommand::ModifiedEvent, this, SLOT(updateGUI()), NULL, 0.0,
        Qt::QueuedConnection);
      }

    // Every time the display updates, it is possible that the arrays available for
    // coloring have changed, hence we reload the list.
    QObject::connect(this->Internal->Representation, SIGNAL(dataUpdated()),
        this, SLOT(needReloadGUI()));
    }
  this->needReloadGUI();
}

//-----------------------------------------------------------------------------
pqPipelineRepresentation* pqDisplayArrayWidget::getRepresentation() const
{
  return this->Internal->Representation;
}

//-----------------------------------------------------------------------------
void pqDisplayArrayWidget::needReloadGUI()
{
  if (this->Internal->Updating)
    {
    return;
    }
  this->Internal->Updating = true;
  pqTimer::singleShot(0, this, SLOT(reloadGUI()));
}

//-----------------------------------------------------------------------------
void pqDisplayArrayWidget::reloadGUI()
{
  this->Internal->Updating = false;
  this->Internal->BlockEmission++;
  this->clear();
  pqPipelineRepresentation* display = this->getRepresentation();
  vtkPVDataInformation* dataInfo = display? display->getInputDataInformation() : NULL;
  vtkPVDataSetAttributesInformation* dsaInfo = dataInfo?
    dataInfo->GetAttributeInformation(vtkDataObject::POINT) : NULL;

  QStringList items;
  if (!this->Internal->ConstantVariableName.isEmpty())
    {
    items << this->Internal->ConstantVariableName;
    }

  if (dsaInfo)
    {
    for (int cc=0, max=dsaInfo->GetNumberOfArrays(); cc < max; ++cc)
      {
      vtkPVArrayInformation* ai = dsaInfo->GetArrayInformation(cc);
      if (ai && ai->GetName())
        {
        items << ai->GetName();
        }
      }
    this->setEnabled(true);
    }
  else
    {
    this->setEnabled(false);
    }
  this->Internal->Variables->insertItems(0, items);
  this->reloadComponents();
  this->updateGUI();
  this->Internal->BlockEmission--;
  emit this->modified();
}

void pqDisplayArrayWidget::setConstantVariableName(const QString& name)
{
  this->Internal->ConstantVariableName = name;
}

const QString& pqDisplayArrayWidget::getConstantVariableName() const
{
  return this->Internal->ConstantVariableName;
}

// Set/Get the name of the property that controls the array name
void pqDisplayArrayWidget::setPropertyArrayName(const QString& name)
{
  this->Internal->PropertyArrayName = name;
}

const QString& pqDisplayArrayWidget::propertyArrayName()
{
  return this->Internal->PropertyArrayName;
}

// Set/Get the name of the property that controals the array component
void pqDisplayArrayWidget::setPropertyArrayComponent(const QString& name)
{
  this->Internal->PropertyArrayComponent = name;
}

const QString& pqDisplayArrayWidget::propertyArrayComponent()
{
  return this->Internal->PropertyArrayComponent;
}

const QString pqDisplayArrayWidget::getArrayName() const
{
  pqPipelineRepresentation* display = this->getRepresentation();
  vtkSMProxy * repr = (display ? display->getProxy() : NULL);
  if (!display || !repr)
    {
    return this->Internal->ConstantVariableName;
    }

  QList<QVariant> list = pqSMAdaptor::getMultipleElementProperty(repr->GetProperty(
      this->Internal->PropertyArrayName.toLatin1().data()));

  if(list.size() < 4)
    {
    return this->Internal->ConstantVariableName;
  }

  QString array = list[4].toString();

  if (array == "")
    {
    return this->Internal->ConstantVariableName;
    }

  return array;
}
