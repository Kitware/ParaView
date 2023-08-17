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
#include <QDebug>
#include <QFileInfo>

//-----------------------------------------------------------------------------
pqTextureComboBox::pqTextureComboBox(
  vtkSMProxyGroupDomain* domain, bool canLoadNew, QWidget* parent)
  : Superclass(parent)
  , Domain(domain)
  , GroupName("")
  , CanLoadNew(canLoadNew)
{
  if (!this->Domain || this->Domain->GetNumberOfGroups() != 1)
  {
    qCritical() << "pqTextureSelectorPropertyWidget can only be used with a ProxyProperty"
                   " with a ProxyGroupDomain containing a single domain with a name.";
    return;
  }
  else
  {
    this->GroupName = this->Domain->GetGroup(0);
  }

  this->updateTextures();
  QObject::connect(this, SIGNAL(currentIndexChanged(int)), this, SLOT(onCurrentIndexChanged(int)));

  // DomainModifiedEvent is never invoked by ProxyGroupDomain
  // https://gitlab.kitware.com/paraview/paraview/-/issues/19062
  // We have to monitor proxy creation
  pqServerManagerObserver* observer = pqApplicationCore::instance()->getServerManagerObserver();
  QObject::connect(observer, SIGNAL(proxyRegistered(const QString&, const QString&, vtkSMProxy*)),
    this, SLOT(proxyRegistered(const QString&, const QString&, vtkSMProxy*)));
  QObject::connect(observer, SIGNAL(proxyUnRegistered(const QString&, const QString&, vtkSMProxy*)),
    this, SLOT(proxyUnRegistered(const QString&, const QString&, vtkSMProxy*)));
}

//-----------------------------------------------------------------------------
pqTextureComboBox::pqTextureComboBox(vtkSMProxyGroupDomain* domain, QWidget* parent)
  : pqTextureComboBox(domain, true, parent)
{
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
  if (group == this->GroupName && this->Domain->IsInDomain(proxy))
  {
    this->updateTextures();
  }
}

//-----------------------------------------------------------------------------
void pqTextureComboBox::proxyUnRegistered(
  const QString& group, const QString& vtkNotUsed(name), vtkSMProxy* vtkNotUsed(proxy))
{
  if (group == this->GroupName)
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
  if (this->CanLoadNew)
  {
    this->addItem("Load ...", QVariant("LOAD"));
  }

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
  if (index == 0)
  {
    Q_EMIT textureChanged(nullptr);
  }
  else if (index == 1 && this->CanLoadNew)
  {
    this->loadTexture();
  }
  else
  {
    vtkSMProxy* texture = reinterpret_cast<vtkSMProxy*>(this->currentData().value<void*>());
    Q_EMIT textureChanged(texture);
  }
}

//-----------------------------------------------------------------------------
void pqTextureComboBox::loadTexture()
{
  QString filters = "Image files (*.png *.jpg *.jpeg *.bmp *.ppm *.tiff *.hdr);;All files (*)";
  pqFileDialog dialog(nullptr, this, tr("Open Texture:"), QString(), filters);
  dialog.setObjectName("LoadTextureDialog");
  dialog.setFileMode(pqFileDialog::ExistingFile);
  if (dialog.exec())
  {
    QStringList files = dialog.getSelectedFiles();
    if (!files.empty())
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
  if (this->GroupName.isEmpty())
  {
    return false;
  }

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
    this->GroupName.toUtf8().data(), finfo.baseName().toUtf8().data(), false);
  pxm->RegisterProxy(this->GroupName.toUtf8().data(), proxyName.c_str(), texture);
  vtkSMPropertyHelper(texture->GetProperty("FileName")).Set(filename.toUtf8().data());
  texture->UpdateVTKObjects();
  texture->Delete();
  this->updateFromTexture(texture);
  return true;
}
