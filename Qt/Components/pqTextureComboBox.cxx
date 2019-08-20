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

// ParaView Includes.
#include "pqApplicationCore.h"
#include "pqFileDialog.h"
#include "pqServerManagerObserver.h"

// Server Manager Includes.
#include "vtkEventQtSlotConnect.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyGroupDomain.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSessionProxyManager.h"

// Qt Includes
#include <QFileInfo>

const std::string pqTextureComboBox::TEXTURES_GROUP = "textures";

//-----------------------------------------------------------------------------
pqTextureComboBox::pqTextureComboBox(vtkSMProxyGroupDomain* domain, QWidget* parent)
  : Superclass(parent)
  , Domain(domain)
{
  this->updateTextures();
  QObject::connect(this, SIGNAL(currentIndexChanged(int)), this, SLOT(onCurrentIndexChanged(int)));

  // DomainModifiedEvent is never invoked by ProxyGroupDomain
  // https://gitlab.kitware.com/paraview/paraview/issues/19062
  // We have to monitor proxy creation
  pqServerManagerObserver* observer = pqApplicationCore::instance()->getServerManagerObserver();
  QObject::connect(observer, SIGNAL(proxyRegistered(const QString&, const QString&, vtkSMProxy*)),
    this, SLOT(proxyRegistered(const QString&, const QString&, vtkSMProxy*)));
  QObject::connect(observer, SIGNAL(proxyUnRegistered(const QString&, const QString&, vtkSMProxy*)),
    this, SLOT(proxyUnRegistered(const QString&, const QString&, vtkSMProxy*)));
}

//-----------------------------------------------------------------------------
void pqTextureComboBox::updateFromTexture(vtkSMProxy* texture)
{
  int index = texture ? this->findData(QVariant::fromValue<void*>(texture)) : 0;
  if (index != -1)
  {
    this->setCurrentIndex(index);
  }
}

//-----------------------------------------------------------------------------
void pqTextureComboBox::proxyRegistered(
  const QString& group, const QString& vtkNotUsed(name), vtkSMProxy* proxy)
{
  if (group == QString(pqTextureComboBox::TEXTURES_GROUP.c_str()) &&
    this->Domain->IsInDomain(proxy))
  {
    this->updateTextures();
  }
}

//-----------------------------------------------------------------------------
void pqTextureComboBox::proxyUnRegistered(
  const QString& group, const QString& vtkNotUsed(name), vtkSMProxy* vtkNotUsed(proxy))
{
  if (group == QString(pqTextureComboBox::TEXTURES_GROUP.c_str()))
  {
    this->updateTextures();
  }
}

//-----------------------------------------------------------------------------
void pqTextureComboBox::updateTextures()
{
  this->blockSignals(true);

  vtkSMProxy* currentTexture = reinterpret_cast<vtkSMProxy*>(this->currentData().value<void*>());

  this->clear();
  this->addItem("None", QVariant("NONE"));
  this->addItem("Load ...", QVariant("LOAD"));

  for (unsigned int i = 0; i < this->Domain->GetNumberOfProxies(); i++)
  {
    const char* proxyName = this->Domain->GetProxyName(i);
    QVariant proxyVar = QVariant::fromValue<void*>(this->Domain->GetProxy(proxyName));
    this->addItem(QString(proxyName), proxyVar);
  }
  this->updateFromTexture(currentTexture);
  this->blockSignals(false);
}

//-----------------------------------------------------------------------------
void pqTextureComboBox::onCurrentIndexChanged(int index)
{
  switch (index)
  {
    case 0:
      emit textureChanged(nullptr);
      break;
    case 1:
      this->loadTexture();
      break;
    default:
      vtkSMProxy* texture = reinterpret_cast<vtkSMProxy*>(this->currentData().value<void*>());
      emit textureChanged(texture);
      break;
  }
}

//-----------------------------------------------------------------------------
void pqTextureComboBox::loadTexture()
{
  QString filters = "Image files (*.png *.jpg *.bmp *.ppm *.tiff);;All files (*)";
  pqFileDialog dialog(0, this, tr("Open Texture:"), QString(), filters);
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
  this->setCurrentIndex(0);
}

//-----------------------------------------------------------------------------
bool pqTextureComboBox::loadTexture(const QString& filename)
{
  QFileInfo finfo(filename);
  if (!finfo.isReadable())
  {
    return false;
  }

  // We must use the vtkSMProxyManager directly so the proxy has correct
  // registered names
  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
  vtkSMSessionProxyManager* spxm = pxm->GetActiveSessionProxyManager();
  vtkSMProxy* texture = pxm->NewProxy("textures", "ImageTexture");
  std::string proxyName = spxm->GetUniqueProxyName(
    pqTextureComboBox::TEXTURES_GROUP.c_str(), finfo.baseName().toUtf8().data(), false);
  pxm->RegisterProxy(pqTextureComboBox::TEXTURES_GROUP.c_str(), proxyName.c_str(), texture);
  vtkSMPropertyHelper(texture->GetProperty("FileName")).Set(filename.toUtf8().data());
  texture->UpdateVTKObjects();
  texture->Delete();
  this->updateFromTexture(texture);
  return true;
}
