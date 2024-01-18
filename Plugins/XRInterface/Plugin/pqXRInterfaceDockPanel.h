// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   pqXRInterfaceDockPanel
 * @brief   PV GUI dock panel for XRInterface
 *
 * This class exists as part of the desktop GUI for ParaView and exposes
 * XRInterface settings and controls.
 */

#include "vtkLogger.h" // for Verbosity enum

#include <QDockWidget>
#include <QScopedPointer>

class pqView;
class vtkPVXMLElement;
class vtkSMProxyLocator;

class pqXRInterfaceDockPanel : public QDockWidget
{
  Q_OBJECT
  typedef QDockWidget Superclass;

public:
  pqXRInterfaceDockPanel(
    const QString& t, QWidget* p = nullptr, Qt::WindowFlags f = Qt::WindowFlags{});
  pqXRInterfaceDockPanel(QWidget* p = nullptr, Qt::WindowFlags f = Qt::WindowFlags{});
  ~pqXRInterfaceDockPanel() override;

protected:
  void attachToCurrentView();
  void sendToXRInterface();
  void showXRView();

  enum XRBackend
  {
    XR_BACKEND_NONE = -1,
    XR_BACKEND_OPENVR = 0,
    XR_BACKEND_OPENXR = 1
  };

protected Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)

  void exportLocationsAsSkyboxes();
  void exportLocationsAsView();

  void defaultCropThicknessChanged(const QString& text);

  void loadState(vtkPVXMLElement*, vtkSMProxyLocator*);
  void saveState(vtkPVXMLElement*);

  void prepareForQuit();

  void beginPlay();
  void endPlay();
  void updateSceneTime();

  void onViewRemoved(pqView*);

  void collaborationConnect();
  void collaborationCallback(std::string const& data, vtkLogger::Verbosity verbosity);

  void xrBackendChanged(int index);

private:
  void constructor();

  struct pqInternals;
  QScopedPointer<pqInternals> Internals;
};
