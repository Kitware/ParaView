/*=========================================================================

   Program: ParaView
   Module:    pqTextureComboBox.cxx

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

========================================================================*/
#include "pqTextureComboBox.h"

// Server Manager Includes.
#include "vtkClientServerID.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkProcessModule.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkSMProxyIterator.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMProxyManager.h"

#include <vtksys/SystemTools.hxx>

// Qt Includes.
#include <QFileInfo>
#include <QIcon>
#include <QPixmap>
#include <QPointer>
#include <QtDebug>
#include <QTimer>

// ParaView Includes.
#include "pqApplicationCore.h"
#include "pqDataRepresentation.h"
#include "pqFileDialog.h"
#include "pqRenderView.h"
#include "pqServerManagerObserver.h"
#include "pqSMAdaptor.h"
#include "pqUndoStack.h"


class pqTextureComboBox::pqInternal
{
public:
  struct Info
    {
    QIcon Icon;
    QString FileName;
    };
  QPointer<pqDataRepresentation> Representation;
  QPointer<pqRenderView> RenderView;
  QMap<vtkSMProxy*, Info> TextureIcons;
  vtkSmartPointer<vtkEventQtSlotConnect> VTKConnect;

  pqInternal()
    {
    this->VTKConnect = vtkSmartPointer<vtkEventQtSlotConnect>::New();
    }
};

#define USEICONS 0
#define TEXTURESGROUP "textures"
//-----------------------------------------------------------------------------
pqTextureComboBox::pqTextureComboBox(QWidget* _parent):Superclass(_parent)
{
  this->Internal = new pqInternal();

  QObject::connect(this, SIGNAL(currentIndexChanged(int)),
    this, SLOT(onActivated(int)));

  pqServerManagerObserver* observer =
    pqApplicationCore::instance()->getServerManagerObserver();
  QObject::connect(observer,
    SIGNAL(proxyRegistered(const QString&, const QString&, vtkSMProxy*)),
    this,
    SLOT(proxyRegistered(const QString&)));
  QObject::connect(observer,
    SIGNAL(proxyUnRegistered(const QString&, const QString&, vtkSMProxy*)),
    this,
    SLOT(proxyUnRegistered(const QString&, const QString&, vtkSMProxy*)));
  this->updateTextures();

  pqUndoStack* stack = pqApplicationCore::instance()->getUndoStack();
  if (stack)
    {
    QObject::connect(this, SIGNAL(begin(const QString&)),
      stack, SLOT(beginUndoSet(const QString&)));
    QObject::connect(this, SIGNAL(end()),
      stack, SLOT(endUndoSet()));
    }
}

//-----------------------------------------------------------------------------
pqTextureComboBox::~pqTextureComboBox()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void pqTextureComboBox::proxyRegistered(const QString& group)
{
  if (group == TEXTURESGROUP)
    {
    this->updateTextures();
    }
}

//-----------------------------------------------------------------------------
void pqTextureComboBox::proxyUnRegistered(const QString& group, const QString&,
  vtkSMProxy* proxy)
{
  if (group == TEXTURESGROUP)
    {
    this->Internal->TextureIcons.remove(proxy);
    QTimer::singleShot(0, this, SLOT(updateTextures()));
    }
}

//-----------------------------------------------------------------------------
void pqTextureComboBox::updateTextures()
{
#if USEICONS
  // Get all proxies under "textures" group and add them to the menu.
  vtkSMProxyIterator* proxyIter = vtkSMProxyIterator::New();
  proxyIter->SetModeToOneGroup();
  for (proxyIter->Begin(TEXTURESGROUP); !proxyIter->IsAtEnd(); proxyIter->Next())
    {
    vtkSMProxy* texture = proxyIter->GetProxy();
    QString filename = pqSMAdaptor::getElementProperty(
      texture->GetProperty("FileName")).toString();
    if (!this->Internal->TextureIcons.contains(texture) ||
      this->Internal->TextureIcons[texture].FileName != filename)
      {
      QPixmap pixmap(filename);
      if (!pixmap.isNull())
        {
        this->Internal->TextureIcons[texture].Icon = QIcon(pixmap.scaled(24, 24));
        this->Internal->TextureIcons[texture].FileName = filename;
        }

      // Connect slot so that when file name changes, we reload the new icon.
      this->Internal->VTKConnect->Disconnect(texture->GetProperty("FileName"));
      this->Internal->VTKConnect->Connect(texture->GetProperty("FileName"),
        vtkCommand::ModifiedEvent, this, SLOT(updateTextures()));
      }
    }
  proxyIter->Delete();
#endif
  this->reload();
}

//-----------------------------------------------------------------------------
void pqTextureComboBox::setRepresentation(pqDataRepresentation* repr)
{
  this->setEnabled(repr != 0);
  if (this->Internal->Representation == repr)
    {
    return;
    }

  if (this->Internal->Representation)
    {
    QObject::disconnect(this->Internal->Representation, 0, this, 0);
    this->Internal->VTKConnect->Disconnect(
      this->Internal->Representation->getProxy()->GetProperty("Texture"));
    }
  this->Internal->Representation = repr;
  if (!this->Internal->Representation)
    {
    return;
    }

  // When the repr is updated, its likely that the available arrays have
  // changed, and texture coords may have become available. Hence, we update the
  // enabled state.
  QObject::connect(this->Internal->Representation, SIGNAL(dataUpdated()),
    this, SLOT(updateEnableState()), Qt::QueuedConnection);

  // When the texture attached to the representation changes, we want to update
  // the combo box.
  if (this->Internal->Representation->getProxy()->GetProperty("Texture"))
    {
    this->Internal->VTKConnect->Connect(
      this->Internal->Representation->getProxy()->GetProperty("Texture"),
      vtkCommand::ModifiedEvent, this, SLOT(updateFromProperty()));
    }
  this->updateFromProperty();

  QTimer::singleShot(0, this, SLOT(updateEnableState()));
}

//-----------------------------------------------------------------------------
void pqTextureComboBox::updateFromProperty()
{
  vtkSMProxy* texture (0x0);
  if(this->Internal->Representation)
    {
    texture = pqSMAdaptor::getProxyProperty(
      this->Internal->Representation->getProxy()->GetProperty("Texture"));
    }
  else
    {
    texture = pqSMAdaptor::getProxyProperty(
      this->Internal->RenderView->getProxy()->GetProperty("BackgroundTexture"));
    }

  this->setCurrentIndex(0);
  if (texture)
    {
    int index = this->findData(texture->GetSelfID().ID);
    if (index != -1)
      {
      this->setCurrentIndex(index);
      }
    }
}

//-----------------------------------------------------------------------------
void pqTextureComboBox::reload()
{
  this->blockSignals(true);
  this->clear();
  this->addItem("None", QVariant("NONE"));
  this->addItem("Load ...", QVariant("LOAD"));

  // Get all proxies under "textures" group and add them to the menu.
  vtkSMProxyIterator* proxyIter = vtkSMProxyIterator::New();
  proxyIter->SetModeToOneGroup();

  QMap<QString, int> countMap;
  for (proxyIter->Begin(TEXTURESGROUP); !proxyIter->IsAtEnd(); proxyIter->Next())
    {
    QString name = proxyIter->GetKey();
    int number = 0;
    if (countMap.contains(name))
      {
      number = countMap[name];
      name = QString("%1 (%2)").arg(name).arg(number);
      }
    countMap[name] = number+1;

    vtkSMProxy* texture = proxyIter->GetProxy();
    if (this->Internal->TextureIcons.contains(texture))
      {
      this->addItem(this->Internal->TextureIcons[texture].Icon,
        name, proxyIter->GetProxy()->GetSelfID().ID);
      }
    else
      {
      this->addItem(name, proxyIter->GetProxy()->GetSelfID().ID);
      }
    }
  proxyIter->Delete();

  this->blockSignals(false);
}

//-----------------------------------------------------------------------------
void pqTextureComboBox::onActivated(int index)
{
  QVariant _data = this->itemData(index);

  vtkSMProxy* proxy (0);
  vtkSMProperty* textureProperty(0);

  if(this->Internal->Representation)
    {
    proxy = this->Internal->Representation->getProxy();
    textureProperty = proxy->GetProperty("Texture");
    }
  else
    {
    proxy = this->Internal->RenderView->getProxy();
    textureProperty = proxy->GetProperty("BackgroundTexture");
    }

  if (!textureProperty)
    {
    qDebug() << "Failed to locate Texture property.";
    return;
    }

  if (_data.toString() == "NONE")
    {
    emit this->begin("Texture Change");
    vtkSMProxyProperty::SafeDownCast(textureProperty)->RemoveAllProxies();
    proxy->UpdateVTKObjects();
    if(this->Internal->Representation)
      {
      this->Internal->Representation->renderView(false);
      }
    else
      {
      this->Internal->RenderView->render();
      }
    emit this->end();
    }
  else if (_data.toString() == "LOAD")
    {
    emit this->begin("Texture Change");
    // Popup load texture dialog.
    this->loadTexture();
    emit this->end();
    }
  else
    {
    // User choose a texture by name, set it on the representation.
    vtkSMProxy* textureProxy = this->getTextureProxy(_data);
    if (!textureProxy)
      {
      qDebug() << "Failed to locate the loaded texture by the name "
        << this->itemText(index);
      return;
      }
    emit this->begin("Texture Change");
    pqSMAdaptor::setProxyProperty(textureProperty, textureProxy);
    proxy->UpdateVTKObjects();
    emit this->end();

    if(this->Internal->Representation)
      {
      this->Internal->Representation->renderView(false);
      }
    else
      {
      this->Internal->RenderView->render();
      }
    }
}

//-----------------------------------------------------------------------------
void pqTextureComboBox::loadTexture()
{
  QString filters = "Image files (*.png *.jpg *.bmp *.ppm *.tiff);;All files (*)";
  pqFileDialog dialog(0, this, tr("Open Texture:"), QString(),
    filters);
  dialog.setObjectName("LoadTextureDialog");
  dialog.setFileMode(pqFileDialog::ExistingFile);
  if (dialog.exec())
    {
    QStringList files = dialog.getSelectedFiles();
    if (files.size() > 0)
      {
      if (this->loadTexture(files[0]))
        {
        return;
        }
      }
    }
  // Since file open failed or no texture was loaded,
  // update the current index in the combo box.
  int index = this->findData("NONE");
  if (index != -1)
    {
    this->setCurrentIndex(index);
    this->onActivated(index);
    }
}

//-----------------------------------------------------------------------------
bool pqTextureComboBox::loadTexture(const QString& filename)
{
  QFileInfo finfo(filename);
  if (!finfo.isReadable())
    {
    return false;
    }

  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
  vtkSMProxy* texture = pxm->NewProxy("textures", "ImageTexture");

  if(this->Internal->Representation)
    {
    texture->SetConnectionID(
      this->Internal->Representation->getProxy()->GetConnectionID());
    }
  else
    {
    texture->SetConnectionID(
      this->Internal->RenderView->getProxy()->GetConnectionID());
    }

  texture->SetServers(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
  pqSMAdaptor::setElementProperty(texture->GetProperty("FileName"), filename);
  pqSMAdaptor::setEnumerationProperty(texture->GetProperty("SourceProcess"),
    "Client");
  texture->UpdateVTKObjects();
  pxm->RegisterProxy(TEXTURESGROUP,
    vtksys::SystemTools::GetFilenameName(filename.toAscii().data()).c_str(),
    texture);
  texture->Delete();

  int index = this->findData(QVariant(texture->GetSelfID().ID));
  if (index != -1)
    {
    this->setCurrentIndex(index);
    this->onActivated(index);
    }
  return true;
}

//-----------------------------------------------------------------------------
vtkSMProxy* pqTextureComboBox::getTextureProxy(const QVariant& _data) const
{
  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
  vtkClientServerID proxyID (_data.value<vtkTypeUInt32>());
  return pxm->GetProxy(TEXTURESGROUP, proxyID);
}

//-----------------------------------------------------------------------------
void pqTextureComboBox::updateEnableState()
{
  bool enabled = false;
  if (this->Internal->Representation)
    {
    // Enable only if we have point texture coordinates.
    vtkPVDataInformation* dataInfo =
      this->Internal->Representation->getRepresentedDataInformation();
    if (!dataInfo)
      {
      return;
      }
     vtkPVDataSetAttributesInformation *pdInfo = dataInfo->GetPointDataInformation();
    if (pdInfo && pdInfo->GetAttributeInformation(vtkDataSetAttributes::TCOORDS))
      {
      enabled = true;
      }
    }
  this->setEnabled(enabled);
  if (!enabled)
    {
    this->setToolTip("No texture coordinates present in the data. Cannot apply texture.");
    }
  else
    {
    this->setToolTip("Select/Load texture to apply.");
    }
}

//-----------------------------------------------------------------------------
void pqTextureComboBox::setRenderView(pqRenderView* rview)
{
  this->setEnabled(rview != 0);
  if (this->Internal->RenderView == rview)
    {
    return;
    }

  if (this->Internal->RenderView)
    {
    QObject::disconnect(this->Internal->RenderView, 0, this, 0);
    this->Internal->VTKConnect->Disconnect(
      this->Internal->RenderView->getProxy()->GetProperty("BackgroundTexture"));
    }
  this->Internal->RenderView = rview;
  if (!this->Internal->RenderView)
    {
    return;
    }

  // When the texture attached to the representation changes, we want to update
  // the combo box.
  this->Internal->VTKConnect->Connect(
    this->Internal->RenderView->getProxy()->GetProperty("BackgroundTexture"),
    vtkCommand::ModifiedEvent, this, SLOT(updateFromProperty()));
  this->updateFromProperty();
}

