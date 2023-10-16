// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class pqBivariateManager
 * @brief Autoload class handling the setup of bivariate representations.
 *
 * This class register default textures for the vtkBivariateTextureRepresentation
 * on startup. These textures are located in the `Resources` folder.
 *
 * This class also handles the rendering loop for the vtkBivariateNoiseRepresentation:
 * It observes every representation of pqRenderView instances when a rendering pass ends.
 * If a representation of type Bivariate Noise Surface is found, then render() is triggered
 * to ensure the next update of the simulation.
 *
 * @sa vtkBivariateNoiseRepresentation vtkBivariateTextureRepresentation
 * pqStreamLinesAnimationManager
 */

#ifndef pqBivariateAnimationManager_h
#define pqBivariateAnimationManager_h

#include <QObject>

#include <set>

class pqObjectBuilder;
class pqServer;
class pqView;

class pqBivariateManager : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  pqBivariateManager(QObject* p = nullptr);
  ~pqBivariateManager() override;

  void onStartup();
  void onShutdown();

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  void onViewAdded(pqView*);
  void onViewRemoved(pqView*);

protected Q_SLOTS:
  void onRenderEnded();
  void onServerAdded(pqServer*);

protected: // NOLINT(readability-redundant-access-specifiers)
  std::set<pqView*> Views;

private:
  Q_DISABLE_COPY(pqBivariateManager)

  void generateTextureProxies(pqServer* server);
};

#endif
