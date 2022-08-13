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
 * @class   pqLookingGlassDockPanel
 * @brief   Dock panel for Looking Glass controls
 *
 * This class exposes settings and controls for the Looking Glass display.
 */

#include <QDockWidget>
#include <QMap>
#include <QPointer>
#include <QString>

class pqRenderView;
class pqView;

class QVTKOpenGLWindow;

class vtkCamera;
class vtkCommand;
class vtkLookingGlassInterface;
class vtkOpenGLRenderWindow;
class vtkRenderWindow;
class vtkSMProxy;
class vtkTextureObject;

class pqLookingGlassDockPanel : public QDockWidget
{
  Q_OBJECT
  typedef QDockWidget Superclass;

public:
  pqLookingGlassDockPanel(
    const QString& t, QWidget* p = nullptr, Qt::WindowFlags f = Qt::WindowFlags{})
    : Superclass(t, p, f)
  {
    this->constructor();
  }

  pqLookingGlassDockPanel(QWidget* p = nullptr, Qt::WindowFlags f = Qt::WindowFlags{})
    : Superclass(p, f)
  {
    this->constructor();
  }

  ~pqLookingGlassDockPanel();

  // Reset the focal distance slider range
  void resetFocalDistanceSliderRange();

protected Q_SLOTS:
  void setView(pqView* view);
  void onRender();

  void onRenderOnLookingGlassClicked();
  void resetToCenterOfRotation();
  void pushFocalPlaneBack();
  void pullFocalPlaneForward();
  void updateEnableStates();
  void saveQuilt();
  void onRecordQuiltClicked();
  void onTargetDeviceChanged(int index);
  void setAttachedDevice(const std::string& deviceType);

  void startRecordingQuilt();
  void stopRecordingQuilt();

  void onFocalDistanceEdited(double position);

  void reset();

  // Invoked when a view is deleted
  void viewRemoved(pqView* view);

  void activeViewChanged(pqView* view);

protected:
  QPointer<pqRenderView> View;
  QPointer<pqRenderView> LastRenderView;

  vtkCommand* EndObserver = nullptr;
  vtkCommand* ViewRenderObserver = nullptr;
  vtkCommand* FocalDistanceObserver = nullptr;
  vtkTextureObject* CopyTexture = nullptr;
  QVTKOpenGLWindow* Widget = nullptr;
  vtkLookingGlassInterface* Interface = nullptr;
  vtkOpenGLRenderWindow* DisplayWindow = nullptr;

  bool RenderNextFrame = false;
  bool IsRecording = false;
  QString MovieFilepath;

  QMap<pqRenderView*, vtkSMProxy*> ViewToSettingsMap;

  vtkSMProxy* getActiveCamera();

  vtkRenderWindow* getRenderWindow();

  // Get the recommended suffix (without the extension) for a quilt
  // file name, based upon the number of tiles. Example: "_qs5x9".
  // This communicates the tile information to the LG software.
  QString getQuiltFileSuffix();

  // Compute the focal distance and direction
  double computeFocalDistanceAndDirection(double directionOfProjection[3]);

  // Get the Looking Glass settings for the displayed view
  vtkSMProxy* getSettingsForView(pqRenderView* view);

  QString getSettingsProxyName(pqView* view);

  void freeDisplayWindowResources();

  class pqInternal;
  pqInternal* Internal = nullptr;

private:
  void constructor();
};
