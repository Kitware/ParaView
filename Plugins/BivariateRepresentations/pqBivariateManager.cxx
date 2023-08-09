// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "pqBivariateManager.h"

#include "pqApplicationCore.h"
#include "pqRenderView.h"
#include "pqRepresentation.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "pqView.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSessionProxyManager.h"

#include "vtkPVBivariatePluginLocation.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMRepresentationProxy.h"

#include <QDebug>
namespace
{
// Nice to have : textures names can be retrieved from the ressources directly.
const std::string TEXTURE_FILES[4]{ "Bremm.png", "Schumann.png", "Steiger.png", "Teulingfig2.png" };
}

//-----------------------------------------------------------------------------
pqBivariateManager::pqBivariateManager(QObject* p)
  : QObject(p)
{
  pqServerManagerModel* smmodel = pqApplicationCore::instance()->getServerManagerModel();
  QObject::connect(smmodel, SIGNAL(preViewAdded(pqView*)), this, SLOT(onViewAdded(pqView*)));
  QObject::connect(smmodel, SIGNAL(preViewRemoved(pqView*)), this, SLOT(onViewRemoved(pqView*)));

  // For client/server mode.
  // Since the plugin is loaded on client before etablishing the server connexion,
  // the texture proxies are created on the builtin server (see onStartup()).
  // Then we can't retrieve them once the connexion is etablished, so we need to
  // register them again on the server once the connexion is etablished.
  QObject::connect(pqApplicationCore::instance()->getServerManagerModel(),
    SIGNAL(serverAdded(pqServer*)), this, SLOT(onServerAdded(pqServer*)));

  // Add currently existing views
  for (pqView* view : smmodel->findItems<pqView*>())
  {
    this->onViewAdded(view);
  }
}

//-----------------------------------------------------------------------------
pqBivariateManager::~pqBivariateManager() = default;

//-----------------------------------------------------------------------------
void pqBivariateManager::onStartup()
{
  pqApplicationCore* core = pqApplicationCore::instance();
  pqServer* activeServer = core->getActiveServer();

  if (!activeServer)
  {
    return;
  }

  this->generateTextureProxies(activeServer);
}

//-----------------------------------------------------------------------------
void pqBivariateManager::onServerAdded(pqServer* server)
{
  this->generateTextureProxies(server);
}

//-----------------------------------------------------------------------------
void pqBivariateManager::onShutdown() {}

//-----------------------------------------------------------------------------
void pqBivariateManager::onRenderEnded()
{
  pqView* view = dynamic_cast<pqView*>(QObject::sender());
  QList<pqRepresentation*> reprs = view->getRepresentations();
  for (int i = 0; i < reprs.count(); ++i)
  {
    vtkSMRepresentationProxy* repr = vtkSMRepresentationProxy::SafeDownCast(reprs[i]->getProxy());
    if (repr && repr->GetProperty("Representation"))
    {
      const char* rs = vtkSMPropertyHelper(repr, "Representation").GetAsString();
      const int visible = vtkSMPropertyHelper(repr, "Visibility").GetAsInt();
      if (rs && !strcmp(rs, "Bivariate Noise Surface") && visible)
      {
        // If the view has a visible bivariate noise representation,
        // then ask for a new render.
        view->render();
        break;
      }
    }
  }
}

//-----------------------------------------------------------------------------
void pqBivariateManager::onViewAdded(pqView* view)
{
  if (dynamic_cast<pqRenderView*>(view))
  {
    this->Views.insert(view);
    QObject::connect(view, SIGNAL(endRender()), this, SLOT(onRenderEnded()));
  }
}

//-----------------------------------------------------------------------------
void pqBivariateManager::onViewRemoved(pqView* view)
{
  if (dynamic_cast<pqRenderView*>(view))
  {
    QObject::disconnect(view, SIGNAL(endRender()), this, SLOT(onRenderEnded()));
    this->Views.erase(view);
  }
}

//-----------------------------------------------------------------------------
void pqBivariateManager::generateTextureProxies(pqServer* server)
{
  std::string texturesFolder;
  std::string pluginLocation = vtkPVBivariatePluginLocation::GetPluginLocation();
  if (!pluginLocation.empty())
  {
    // get the path
    pluginLocation = vtksys::SystemTools::GetFilenamePath(pluginLocation);
    texturesFolder = pluginLocation + "/Resources/";
  }
  else
  {
    // This can happend e.g. if ParaView is statically build
    qWarning() << "Unable to retrieve the BivariateRepresentations plugin location - default "
                  "textures can't be retrieved.";
    return;
  }

  for (auto& textureFile : TEXTURE_FILES)
  {
    auto texturePath = texturesFolder + textureFile;

    // Create texture proxies. We do not use pqObjectBuilder because we need to specify
    // a custom proxy name (which is the name of the texture file, minus the extension)
    vtkSMSessionProxyManager* spxm = server->proxyManager();
    vtkSMProxy* textureProxy = spxm->NewProxy("textures", "ImageTexture");
    auto textureName = vtksys::SystemTools::GetFilenameWithoutExtension(textureFile);
    spxm->RegisterProxy("textures", textureName.c_str(), textureProxy);
    vtkSMPropertyHelper(textureProxy->GetProperty("FileName")).Set(texturePath.c_str());
    textureProxy->UpdateVTKObjects();
    textureProxy->Delete();
  }
}
