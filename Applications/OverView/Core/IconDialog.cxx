/*=========================================================================

   Program: ParaView
   Module:    IconDialog.cxx

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
#include "IconDialog.h"
#include "ui_IconDialog.h"

#include <pqApplicationCore.h>
#include <pqRepresentation.h>
#include <pqImageUtil.h>
#include <pqPropertyHelper.h>
#include <pqSettings.h>

#include <vtkAbstractArray.h>
#include <vtkDataSetAttributes.h>
#include <vtkExtractVOI.h>
#include <vtkGraph.h>
#include <vtkImageData.h>
#include <vtkProcessModule.h>
#include <vtkSmartPointer.h>
#include <vtkTexture.h>

#include <vtkSMClientDeliveryRepresentationProxy.h>
#include <vtkSMEnumerationDomain.h>
#include <vtkSMIntVectorProperty.h>
#include <vtkSMProxy.h>
#include <vtkSMProxyProperty.h>
#include <vtkSMProxyManager.h>
#include <vtkSMSourceProxy.h>
#include <vtkSMStringVectorProperty.h>

#include <QList>
#include <QSet>
#include <QHeaderView>
#include <QIcon>
#include <QMessageBox>
#include <QPixmap>
#include <QImage>

#include <QtDebug>

#include <vtksys/SystemTools.hxx>

/////////////////////////////////////////////////////////////////////////
// IconDialog::pqImplementation

class IconDialog::pqImplementation
{
public:
  pqImplementation(
    pqRepresentation *representation) :
    IconTexture(0),
    Representation(representation)
  {
  }

  ~pqImplementation()
  {
    if(this->IconTexture)
      {
      this->IconTexture->Delete();
      this->IconTexture = 0;
      }
  }

  vtkSMSourceProxy *IconTexture;
  
  QList<QIcon> Icons;
  int ArrayType;

  Ui::IconDialog UI;
  pqRepresentation *Representation;
};

/////////////////////////////////////////////////////////////////////////
// IconDialog

IconDialog::IconDialog(
  pqRepresentation *representation,
  QWidget* widget_parent) :
    pqDialog(widget_parent),
    Implementation(new pqImplementation(representation))
{
  this->Implementation->UI.setupUi(this);

  this->Implementation->UI.table->verticalHeader()->setVisible(false);
  this->Implementation->UI.table->setColumnWidth(0,130);
  this->Implementation->UI.table->setColumnWidth(1,130);

  this->Implementation->UI.iconFile->setForceSingleFile(true);

  // Call this before we set up the connections
  this->initializeDisplay();

  QObject::connect(this->Implementation->UI.iconArrayName, SIGNAL(currentIndexChanged(const QString&)),
    this, SLOT(updateDisplay()));

  QObject::connect(this->Implementation->UI.iconSize, SIGNAL(valueChanged(int)),
    this, SLOT(onIconSizeChanged()));

  QObject::connect(this->Implementation->UI.applyIconSize, SIGNAL(pressed()),
    this, SLOT(onApplyIconSize()));

  QObject::connect(this->Implementation->UI.iconFile, SIGNAL(filenameChanged(const QString&)),
    this, SLOT(readIconSheetFromFile(const QString&)));
}

IconDialog::~IconDialog()
{
  delete this->Implementation;
}

void IconDialog::initializeDisplay()
{
  vtkSMClientDeliveryRepresentationProxy* const proxy = vtkSMClientDeliveryRepresentationProxy::SafeDownCast(this->Implementation->Representation->getProxy());
  vtkGraph* const graph = vtkGraph::SafeDownCast(proxy->GetOutput());

  for(vtkIdType i=0; i<graph->GetVertexData()->GetNumberOfArrays(); i++)
    {
    this->Implementation->UI.iconArrayName->addItem(graph->GetVertexData()->GetArrayName(i));
    }

  this->Implementation->UI.iconArrayName->setCurrentIndex(this->Implementation->UI.iconArrayName->findText(pqPropertyHelper(proxy, "IconArray").GetAsString()));
  this->Implementation->UI.iconSize->setValue(vtkSMPropertyHelper(proxy, "IconSize").GetAsInt());
  this->Implementation->UI.iconFile->setSingleFilename(pqPropertyHelper(proxy, "IconFile").GetAsString());

  if(this->Implementation->UI.iconFile->singleFilename().isEmpty())
    {
    return;
    }

  this->readIconSheetFromFile(this->Implementation->UI.iconFile->singleFilename());

  vtkSMStringVectorProperty *iconTypes = vtkSMStringVectorProperty::SafeDownCast(proxy->GetProperty("IconTypes"));
  vtkSMIntVectorProperty *iconIndices = vtkSMIntVectorProperty::SafeDownCast(proxy->GetProperty("IconIndices"));
  for(unsigned int j=0; j<iconTypes->GetNumberOfElements(); j++)
    {
    QString type = iconTypes->GetElement(j);
    for(int row=0; row<this->Implementation->UI.table->rowCount(); row++)
      {
      if(type == this->Implementation->UI.table->item(row,0)->text())
        {
        QComboBox *iconSelector = dynamic_cast<QComboBox*>(this->Implementation->UI.table->cellWidget(row,1));
        iconSelector->setCurrentIndex(iconIndices->GetElement(j)+1);
        break;
        }
      }
    }
}

void IconDialog::acceptInternal()
{
  if(!this->Implementation->IconTexture)
    {
    return;
    }

  vtkSMProxy* const proxy = vtkSMProxy::SafeDownCast(this->Implementation->Representation->getProxy());

  pqPropertyHelper(proxy, "IconFile").Set(this->Implementation->UI.iconFile->singleFilename());
  pqPropertyHelper(proxy, "IconArray").Set(this->Implementation->UI.iconArrayName->currentText());
  vtkSMPropertyHelper(proxy, "IconSize").Set(this->Implementation->UI.iconSize->value());
  vtkSMProxyProperty *prop = vtkSMProxyProperty::SafeDownCast(proxy->GetProperty("IconTexture"));
  prop->RemoveAllProxies();
  prop->AddProxy(this->Implementation->IconTexture);

  vtkSMStringVectorProperty *iconTypes = vtkSMStringVectorProperty::SafeDownCast(proxy->GetProperty("IconTypes"));
  vtkSMIntVectorProperty *iconIndices = vtkSMIntVectorProperty::SafeDownCast(proxy->GetProperty("IconIndices"));
  int count = 0;
  for(int row=0; row<this->Implementation->UI.table->rowCount(); row++)
    {
    QComboBox *iconSelector = dynamic_cast<QComboBox*>(this->Implementation->UI.table->cellWidget(row,1));
    if(iconSelector->currentText() == "None")
      {
      continue;
      }
    QString type = this->Implementation->UI.table->item(row,0)->text();
    iconTypes->SetElement(count, type.toAscii().data());
    iconIndices->SetElement(count, iconSelector->currentIndex()-1);
    count++;
    }

  proxy->UpdateVTKObjects();
}

void IconDialog::updateDisplay()
{
  vtkSMClientDeliveryRepresentationProxy* const proxy = vtkSMClientDeliveryRepresentationProxy::SafeDownCast(this->Implementation->Representation->getProxy());
  vtkGraph* const graph = vtkGraph::SafeDownCast(proxy->GetOutput());

  vtkAbstractArray *vals = graph->GetVertexData()->GetAbstractArray(this->Implementation->UI.iconArrayName->currentText().toAscii().data());
  if(!vals)
    {
    return;
    }

  switch(vals->GetDataType())
    {
    case VTK_INT:
      this->Implementation->ArrayType = vtkSMStringVectorProperty::INT;
      break;
    default:
      this->Implementation->ArrayType = vtkSMStringVectorProperty::STRING;
    }

  QSet<QString> uniqueValuesSet;
  bool maxSizeReached = false;
  for(vtkIdType aidx=0; aidx<vals->GetNumberOfTuples(); aidx++)
    {
    vtkVariant v;
    switch(vals->GetDataType())
      {
      vtkExtraExtendedTemplateMacro(v = *static_cast<VTK_TT*>(vals->GetVoidPointer(aidx)));
      }
    uniqueValuesSet.insert(v.ToString().c_str());
    if(uniqueValuesSet.size() > 500)
      {
      maxSizeReached = true;
      break;
      }
    }

  if(maxSizeReached)
    {
    QMessageBox::warning(this,"Warning","The array you've selected contains too many unique values.");
    return;
    }

  this->Implementation->UI.table->clearContents();
  QList<QString> uniqueValues = QList<QString>::fromSet(uniqueValuesSet);
  this->Implementation->UI.table->setRowCount(uniqueValues.size());
  for(int j=0; j<uniqueValues.size(); j++)
    {
    QTableWidgetItem *type = new QTableWidgetItem(uniqueValues.at(j));
    this->Implementation->UI.table->setItem(j,0,type);
    QComboBox *iconSelector = new QComboBox();
    iconSelector->addItem("None");
    for(int i=0; i<this->Implementation->Icons.size(); i++)
      {
      iconSelector->addItem(this->Implementation->Icons.at(i),"");
      }
    this->Implementation->UI.table->setCellWidget(j,1,iconSelector);
    }

  //this->Implementation->UI.table->sortItems(0);
}

void IconDialog::onIconSizeChanged()
{
  this->Implementation->UI.applyIconSize->setEnabled(true);
}

void IconDialog::onApplyIconSize()
{
  QString filename = this->Implementation->UI.iconFile->singleFilename();
  if(!filename.isEmpty())
    {
    this->readIconSheetFromFile(filename);
    }

  this->Implementation->UI.applyIconSize->setEnabled(false);
}

void IconDialog::readIconSheetFromFile(const QString &filename)
{
  this->Implementation->Icons.clear();

  vtkSMProxy* const proxy = vtkSMProxy::SafeDownCast(this->Implementation->Representation->getProxy());
  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();

  if(this->Implementation->IconTexture)
    {
    //pxm->UnRegisterProxy(this->Implementation->IconTexture);
    this->Implementation->IconTexture->Delete();
    this->Implementation->IconTexture = 0;
    }

  this->Implementation->IconTexture = vtkSMSourceProxy::SafeDownCast(pxm->NewProxy("textures", "ImageTexture"));
  this->Implementation->IconTexture->SetConnectionID(proxy->GetConnectionID());
  this->Implementation->IconTexture->SetServers(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
  pqPropertyHelper(this->Implementation->IconTexture, "FileName").Set(filename.toAscii().data());
  vtkSMPropertyHelper(this->Implementation->IconTexture, "SourceProcess").Set(vtkProcessModule::CLIENT);
  this->Implementation->IconTexture->UpdateVTKObjects();

  pxm->RegisterProxy("textures", 
      vtksys::SystemTools::GetFilenameName(filename.toAscii().data()).c_str(),
    this->Implementation->IconTexture);

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkTexture *texture = vtkTexture::SafeDownCast(pm->GetObjectFromID(this->Implementation->IconTexture->GetID()));

  vtkExtractVOI *imageSubsample = vtkExtractVOI::New();
  int iconSize = this->Implementation->UI.iconSize->value();
  texture->GetInput()->Update();
  imageSubsample->SetInput(texture->GetInput());
  int *sheetSize = texture->GetInput()->GetDimensions();
  int dimX = sheetSize[0]/iconSize;
  int dimY = sheetSize[1]/iconSize;
  for(int k=dimY-1; k>=0; k--)
    {
    for(int j=0; j<dimX; j++)
      {
      int subX = j*iconSize;
      int subY = k*iconSize;
      int subImage[6] = {subX,subX+iconSize,subY,subY+iconSize,0,0};
      imageSubsample->SetVOI(subImage);
      imageSubsample->Update();
      QImage image;
      pqImageUtil::fromImageData(imageSubsample->GetOutput(), image);
      this->Implementation->Icons.push_back(QIcon(QPixmap::fromImage(image)));
      }
    }

  this->updateDisplay();

  imageSubsample->Delete();
}


