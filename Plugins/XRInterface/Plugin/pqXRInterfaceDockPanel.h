/*=========================================================================

  Program:   ParaView

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   pqXRInterfaceDockPanel
 * @brief   PV GUI dock panel for XRInterface
 *
 * This class exists as part of the desktop GUI for ParaView and exposes
 * XRInterface settings and controls.
 */

#include "vtkLogger.h" // for Verbosity enum
#include <QDockWidget>

class pqXRInterfaceControls;
class pqView;
class vtkPVXMLElement;
class vtkSMProxyLocator;
class vtkPVXRInterfaceHelper;

class pqXRInterfaceDockPanel : public QDockWidget
{
  Q_OBJECT
  typedef QDockWidget Superclass;

public:
  pqXRInterfaceDockPanel(
    const QString& t, QWidget* p = nullptr, Qt::WindowFlags f = Qt::WindowFlags{})
    : Superclass(t, p, f)
  {
    this->constructor();
  }
  pqXRInterfaceDockPanel(QWidget* p = nullptr, Qt::WindowFlags f = Qt::WindowFlags{})
    : Superclass(p, f)
  {
    this->constructor();
  }

  ~pqXRInterfaceDockPanel() override;

protected:
  vtkPVXRInterfaceHelper* Helper;
  pqXRInterfaceControls* XRInterfaceControls;
  void attachToCurrentView();
  void sendToXRInterface();
  void showXRView();

protected Q_SLOTS:

  void exportLocationsAsSkyboxes();
  void exportLocationsAsView();

  void defaultCropThicknessChanged(const QString& text);

  void setActiveView(pqView*);

  void loadState(vtkPVXMLElement*, vtkSMProxyLocator*);
  void saveState(vtkPVXMLElement*);

  void prepareForQuit();

  void beginPlay();
  void endPlay();
  void updateSceneTime();

  void onViewAdded(pqView*);
  void onViewRemoved(pqView*);

  void collaborationConnect();
  void collaborationCallback(std::string const& data, vtkLogger::Verbosity verbosity);

  void editableFieldChanged(const QString& text);
  void fieldValuesChanged(const QString& text);

private:
  void constructor();

  class pqInternals;
  pqInternals* Internals;
};
