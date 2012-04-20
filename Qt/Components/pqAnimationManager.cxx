/*=========================================================================

   Program: ParaView
   Module:    pqAnimationManager.cxx

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
#include "pqAnimationManager.h"
#include "ui_pqAbortAnimation.h"
#include "ui_pqAnimationSettings.h"

#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkPVServerInformation.h"
#include "vtkPVXMLElement.h"
#include "vtkProcessModule.h"
#include "vtkRenderWindow.h"
#include "vtkSMAnimationSceneGeometryWriter.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyIterator.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMSession.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSMViewProxy.h"
#include "vtkSmartPointer.h"

#include <QIntValidator>
#include <QFileInfo>
#include <QMap>
#include <QMessageBox>
#include <QPointer>
#include <QSize>
#include <QtDebug>

#include "pqAnimationCue.h"
#include "pqAnimationScene.h"
#include "pqAnimationSceneImageWriter.h"
#include "pqApplicationCore.h"
#include "pqCoreUtilities.h"
#include "pqEventDispatcher.h"
#include "pqFileDialog.h"
#include "pqObjectBuilder.h"
#include "pqProgressManager.h"
#include "pqProxy.h"
#include "pqRenderViewBase.h"
#include "pqSMAdaptor.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "pqSettings.h"
#include "pqTabbedMultiViewWidget.h"

#include <vtksys/ios/sstream>

#define SEQUENCE 0
#define REALTIME 1
#define SNAP_TO_TIMESTEPS 2

//-----------------------------------------------------------------------------
class pqAnimationManager::pqInternals
{
public:
  QPointer<pqServer> ActiveServer;
  typedef QMap<pqServer*, QPointer<pqAnimationScene> > SceneMap;
  SceneMap Scenes;
  Ui::pqAnimationSettingsDialog* AnimationSettingsDialog;

  QSize OldMaxSize;
  QSize OldSize;

  double AspectRatio;
};

//-----------------------------------------------------------------------------
pqAnimationManager::pqAnimationManager(QObject* _parent/*=0*/) 
:QObject(_parent)
{
  this->Internals = new pqAnimationManager::pqInternals();
  pqServerManagerModel* smmodel = 
    pqApplicationCore::instance()->getServerManagerModel();
  QObject::connect(smmodel, SIGNAL(proxyAdded(pqProxy*)),
    this, SLOT(onProxyAdded(pqProxy*)));
  QObject::connect(smmodel, SIGNAL(proxyRemoved(pqProxy*)),
    this, SLOT(onProxyRemoved(pqProxy*)));

  QObject::connect(smmodel, SIGNAL(viewAdded(pqView*)),
    this, SLOT(updateViewModules()));
  QObject::connect(smmodel, SIGNAL(viewRemoved(pqView*)),
    this, SLOT(updateViewModules()));
  
  this->restoreSettings();
}

//-----------------------------------------------------------------------------
pqAnimationManager::~pqAnimationManager()
{
  this->saveSettings();
  delete this->Internals;
}

//-----------------------------------------------------------------------------
void pqAnimationManager::updateViewModules()
{
  pqAnimationScene* scene = this->getActiveScene();
  if (!scene)
    {
    return;
    }

  QList<pqView*> viewModules = 
    pqApplicationCore::instance()->getServerManagerModel()->
    findItems<pqView*>(this->Internals->ActiveServer);
  
  QList<pqSMProxy> viewList;
  foreach(pqView* view, viewModules)
    {
    viewList.push_back(pqSMProxy(view->getProxy()));
    } 

  emit this->beginNonUndoableChanges();

  vtkSMProxy* sceneProxy = scene->getProxy();
  pqSMAdaptor::setProxyListProperty(sceneProxy->GetProperty("ViewModules"),
    viewList);
  sceneProxy->UpdateProperty("ViewModules");

  emit this->endNonUndoableChanges();
}

//-----------------------------------------------------------------------------
void pqAnimationManager::onProxyAdded(pqProxy* proxy)
{
  pqAnimationScene* scene = qobject_cast<pqAnimationScene*>(proxy);
  if (scene && !this->Internals->Scenes.contains(scene->getServer()))
    {
    this->Internals->Scenes[scene->getServer()] = scene;
    if (this->Internals->ActiveServer == scene->getServer())
      {
      emit this->activeSceneChanged(this->getActiveScene());
      }
    }
}

//-----------------------------------------------------------------------------
void pqAnimationManager::onProxyRemoved(pqProxy* proxy)
{
  pqAnimationScene* scene = qobject_cast<pqAnimationScene*>(proxy);
  if (scene) 
    {
    this->Internals->Scenes.remove(scene->getServer());
    if (this->Internals->ActiveServer == scene->getServer())
      {
      emit this->activeSceneChanged(this->getActiveScene());
      }
    }
}

//-----------------------------------------------------------------------------
void pqAnimationManager::onActiveServerChanged(pqServer* server)
{
  // In case of multi-server that method can be called when we disconnect
  // from one or our connected server.
  // Check if the server is going to be deleted and if so just skip creation
  if(!server || !server->session() || server->session()->GetReferenceCount() == 1)
    {
    return;
    }

  this->Internals->ActiveServer = server;
  if (server && !this->getActiveScene())
    {
    this->createActiveScene();
    }
  emit this->activeSceneChanged(this->getActiveScene());
}

//-----------------------------------------------------------------------------
pqAnimationScene* pqAnimationManager::getActiveScene() const
{
  return this->getScene(this->Internals->ActiveServer);
}

//-----------------------------------------------------------------------------
pqAnimationScene* pqAnimationManager::getScene(pqServer* server) const
{
  if (server && this->Internals->Scenes.contains(server))
    {
    return this->Internals->Scenes.value(server);
    }
  return 0;
}

//-----------------------------------------------------------------------------
pqAnimationScene* pqAnimationManager::createActiveScene() 
{
  if (this->Internals->ActiveServer)
    {
    pqObjectBuilder* builder = 
      pqApplicationCore::instance()->getObjectBuilder();
    pqAnimationScene* scene = builder->createAnimationScene(
      this->Internals->ActiveServer);
    
    // this will result in a call to onProxyAdded() and proper
    // signals will be emitted.
    if (!scene)
      {
      qDebug() << "Failed to create scene proxy.";
      }
    this->updateViewModules();
    return this->getActiveScene();
    }
  return 0;
}


//-----------------------------------------------------------------------------
pqAnimationCue* pqAnimationManager::getCue(
  pqAnimationScene* scene, vtkSMProxy* proxy, const char* propertyname, 
  int index) const
{
  return (scene? scene->getCue(proxy, propertyname, index) : 0);
}

//-----------------------------------------------------------------------------
// Called when user changes some property on the save animation dialog.
void pqAnimationManager::updateGUI()
{
  double framerate =
    this->Internals->AnimationSettingsDialog->frameRate->value();
  int num_frames = 
    this->Internals->AnimationSettingsDialog->spinBoxNumberOfFrames->value();
  double duration =  
    this->Internals->AnimationSettingsDialog->animationDuration->value();
  int frames_per_timestep =
    this->Internals->AnimationSettingsDialog->spinBoxFramesPerTimestep->value();

  vtkSMProxy* sceneProxy = this->getActiveScene()->getProxy();
  int playMode = pqSMAdaptor::getElementProperty(
    sceneProxy->GetProperty("PlayMode")).toInt();

  switch (playMode)
    {
  case SNAP_TO_TIMESTEPS:
    // get original number of frames.
    num_frames = pqSMAdaptor::getMultipleElementProperty(
      sceneProxy->GetProperty("TimeSteps")).size();
    num_frames = frames_per_timestep*num_frames;
    this->Internals->AnimationSettingsDialog->spinBoxNumberOfFrames->
      blockSignals(true);
    this->Internals->AnimationSettingsDialog->spinBoxNumberOfFrames->
      setValue(num_frames);
    this->Internals->AnimationSettingsDialog->spinBoxNumberOfFrames->
      blockSignals(false);
    // don't break, let it fall through to SEQUENCE.

  case SEQUENCE:
    this->Internals->AnimationSettingsDialog->animationDuration->
      blockSignals(true);
    this->Internals->AnimationSettingsDialog->animationDuration->setValue(
      num_frames/framerate);
    this->Internals->AnimationSettingsDialog->animationDuration->
      blockSignals(false);
    break;

  case REALTIME:
    this->Internals->AnimationSettingsDialog->spinBoxNumberOfFrames->
      blockSignals(true);
    this->Internals->AnimationSettingsDialog->spinBoxNumberOfFrames->setValue(
      static_cast<int>(duration*framerate));
    this->Internals->AnimationSettingsDialog->spinBoxNumberOfFrames->
      blockSignals(false);
    break;
    }
}

//-----------------------------------------------------------------------------
void pqAnimationManager::onWidthEdited()
{
  Ui::pqAnimationSettingsDialog *dialog
    = this->Internals->AnimationSettingsDialog;
  if (dialog->lockAspect->isChecked())
    {
    int width = dialog->width->text().toInt();
    int height = static_cast<int>(width/this->Internals->AspectRatio);
    dialog->height->setText(QString::number(height));
    }
}

//-----------------------------------------------------------------------------
void pqAnimationManager::onHeightEdited()
{
  Ui::pqAnimationSettingsDialog *dialog
    = this->Internals->AnimationSettingsDialog;
  if (dialog->lockAspect->isChecked())
    {
    int height = dialog->height->text().toInt();
    int width = static_cast<int>(height*this->Internals->AspectRatio);
    dialog->width->setText(QString::number(width));
    }
}

//-----------------------------------------------------------------------------
void pqAnimationManager::onLockAspectRatio(bool lock)
{
  if (lock)
    {
    Ui::pqAnimationSettingsDialog *dialog
      = this->Internals->AnimationSettingsDialog;
    int width = dialog->width->text().toInt();
    int height = dialog->height->text().toInt();
    this->Internals->AspectRatio
      = static_cast<double>(width)/static_cast<double>(height);
    }
}

//-----------------------------------------------------------------------------
#define PADDING_COMPENSATION QSize(16, 16);
inline void enforceMultiple4(QSize& newSize)
{
  QSize requested_newSize = newSize;
  int &width = newSize.rwidth();
  int &height = newSize.rheight();
  if ((width % 4) > 0)
    {
    width -= width % 4;
    }
  if ((height % 4) > 0)
    {
    height -= height % 4;
    }

  if (requested_newSize != newSize)
    {
    QMessageBox::warning(NULL, "Resolution Changed",
      QString("The requested resolution has been changed from (%1, %2)\n").arg(
        requested_newSize.width()).arg(requested_newSize.height()) + 
      QString("to (%1, %2) to match format specifications.").arg(
        newSize.width()).arg(newSize.height()));
    }
}

//-----------------------------------------------------------------------------
bool pqAnimationManager::saveAnimation()
{
  pqAnimationScene* scene = this->getActiveScene();
  if (!scene)
    {
    return false;
    }
  // Ensure that GUI is up-to-date so that we get correct sizes.
  pqEventDispatcher::processEventsAndWait(1);
  vtkSMProxy* sceneProxy = scene->getProxy();

  QDialog dialog;
  Ui::pqAnimationSettingsDialog dialogUI;
  this->Internals->AnimationSettingsDialog = &dialogUI;
  dialogUI.setupUi(&dialog);

  QIntValidator *intValidator = new QIntValidator(this);
  intValidator->setBottom(50);
  dialogUI.width->setValidator(intValidator);
  intValidator = new QIntValidator(this);
  intValidator->setBottom(50);
  dialogUI.height->setValidator(intValidator);

  // Cannot disconnect and save animation unless connected to a remote server.
  dialogUI.checkBoxDisconnect->setEnabled(
        this->Internals->ActiveServer->isRemote() &&
        !this->Internals->ActiveServer->session()->IsMultiClients());

  // Use viewManager is available.
  pqTabbedMultiViewWidget* viewManager = qobject_cast<pqTabbedMultiViewWidget*>(
    pqApplicationCore::instance()->manager("MULTIVIEW_WIDGET"));
  
  // Set current size of the window.
  QSize viewSize = viewManager? viewManager->clientSize() : QSize(800, 600);
  // to avoid some unpredicable padding issues, I am reducing the size by a few
  // pixels.
  QSize padding = PADDING_COMPENSATION;
  if (viewSize.width() > viewSize.height())
    {
    padding.setWidth((padding.width()*viewSize.width())/viewSize.height());
    }
  else
    {
    padding.setHeight((padding.height()*viewSize.height())/viewSize.width());
    }
  viewSize -= padding;
  dialogUI.height->setText(QString::number(viewSize.height()));
  dialogUI.width->setText(QString::number(viewSize.width()));

  // Frames per timestep is only shown
  // when saving in SNAP_TO_TIMESTEPS mode.
  dialogUI.spinBoxFramesPerTimestep->hide();
  dialogUI.labelFramesPerTimestep->hide();

  int playMode = pqSMAdaptor::getElementProperty(
    sceneProxy->GetProperty("PlayMode")).toInt();

  // Set current duration/frame rate/no. of. frames.
  double frame_rate = dialogUI.frameRate->value();
  // Save the current player property values.
  int num_frames = pqSMAdaptor::getElementProperty(
    sceneProxy->GetProperty("NumberOfFrames")).toInt();
  int duration = pqSMAdaptor::getElementProperty(
    sceneProxy->GetProperty("Duration")).toInt();
  int num_steps = pqSMAdaptor::getMultipleElementProperty(
    sceneProxy->GetProperty("TimeSteps")).size();
  int frames_per_timestep = pqSMAdaptor::getElementProperty(
    sceneProxy->GetProperty("FramesPerTimestep")).toInt();

  switch (playMode)
    {
  case SEQUENCE:
    dialogUI.spinBoxNumberOfFrames->setValue(num_frames);
    dialogUI.animationDuration->setEnabled(false);
    dialogUI.animationDuration->setValue(num_frames/frame_rate);
    break;

  case REALTIME:
    dialogUI.animationDuration->setValue(duration);
    dialogUI.spinBoxNumberOfFrames->setValue(
      static_cast<int>(duration*frame_rate));
    dialogUI.spinBoxNumberOfFrames->setEnabled(false);
    break;

  case SNAP_TO_TIMESTEPS:
    dialogUI.spinBoxNumberOfFrames->setValue(num_steps);
    dialogUI.animationDuration->setValue(num_steps*frame_rate);
    dialogUI.spinBoxNumberOfFrames->setEnabled(false);
    dialogUI.animationDuration->setEnabled(false);
    dialogUI.spinBoxFramesPerTimestep->show();
    dialogUI.spinBoxFramesPerTimestep->setValue(frames_per_timestep);
    dialogUI.labelFramesPerTimestep->show();
    break;
    }

  QObject::connect(
    dialogUI.animationDuration, SIGNAL(valueChanged(double)),
    this, SLOT(updateGUI()));
  QObject::connect(
    dialogUI.frameRate, SIGNAL(valueChanged(double)),
    this, SLOT(updateGUI()));
  QObject::connect(
    dialogUI.spinBoxNumberOfFrames, SIGNAL(valueChanged(int)),
    this, SLOT(updateGUI()));
  QObject::connect(
    dialogUI.spinBoxFramesPerTimestep, SIGNAL(valueChanged(int)),
    this, SLOT(updateGUI()));

  QObject::connect(dialogUI.width, SIGNAL(editingFinished()),
                   this, SLOT(onWidthEdited()));
  QObject::connect(dialogUI.height, SIGNAL(editingFinished()),
                   this, SLOT(onHeightEdited()));
  QObject::connect(dialogUI.lockAspect, SIGNAL(toggled(bool)),
                   this, SLOT(onLockAspectRatio(bool)));

  if (!dialog.exec())
    {
    this->Internals->AnimationSettingsDialog = 0;
    return false;
    }
  this->Internals->AnimationSettingsDialog = 0;

  bool disconnect_and_save = 
    (dialogUI.checkBoxDisconnect->checkState() == Qt::Checked);
  int stereo = dialogUI.stereoMode->currentIndex();
  if (stereo)
    {
    QString stereoMode = dialogUI.stereoMode->currentText();
    if (stereoMode == "Red-Blue")
      {
      stereo = VTK_STEREO_RED_BLUE;
      }
    else if (stereoMode == "Interlaced")
      {
      stereo = VTK_STEREO_INTERLACED;
      }
    else if (stereoMode == "Checkerboard")
      {
      stereo = VTK_STEREO_CHECKERBOARD;
      }
    else if (stereoMode == "Left Eye Only")
      {
      stereo = VTK_STEREO_LEFT;
      }
    else if (stereoMode == "Right Eye Only")
      {
      stereo = VTK_STEREO_RIGHT;
      }
    else
      {
      stereo = 0;
      }
    }

  // Now obtain filename for the animation.
  vtkSmartPointer<vtkPVServerInformation> serverInfo;
  if (disconnect_and_save)
    {
    serverInfo = sceneProxy->GetSession()->GetServerInformation();
    if (!serverInfo)
      {
      qWarning() << "Failed to locate server information about AVI support.";
      disconnect_and_save = false;
      }
    }
  else
    {
    // vtkPVServerInformation initialize AVI support in constructor for the
    // local process.
    serverInfo = vtkSmartPointer<vtkPVServerInformation>::New();
    }

  QString filters = "";
  if (serverInfo && serverInfo->GetOGVSupport())
    {
    filters += "Ogg/Theora files (*.ogv);;";
    }
  if (serverInfo && serverInfo->GetAVISupport())
    {
    filters += "AVI files (*.avi);;";
    }
  filters +="JPEG images (*.jpg);;TIFF images (*.tif);;PNG images (*.png);;";
  filters +="All files(*)";

  QWidget* parent_window = pqCoreUtilities::mainWidget();

  // Create a server dialog is disconnect-and-save is true, else create a client
  // dialog.
  pqFileDialog *file_dialog = new pqFileDialog(
    disconnect_and_save?  scene->getServer() : 0,
    parent_window,
    tr("Save Animation"), QString(), filters);
  file_dialog->setRecentlyUsedExtension(this->AnimationExtension);
  file_dialog->setObjectName("FileSaveAnimationDialog");
  file_dialog->setFileMode(pqFileDialog::AnyFile);
  if (file_dialog->exec() != QDialog::Accepted)
    {
    delete file_dialog;
    return false;
    }

  QStringList files  = file_dialog->getSelectedFiles();
  QFileInfo fileInfo = QFileInfo( files[0] );
  this->AnimationExtension = QString("*.") + fileInfo.suffix();
  
  // essential to destroy file dialog, before we disconnect from the server, if
  // at all.
  delete file_dialog;

  if (files.size() == 0)
    {
    return false;
    }

  QString filename = files[0];

  // Update Scene properties based on user options. 
  emit this->beginNonUndoableChanges();

  if (stereo)
    {
    pqRenderViewBase::setStereo(stereo);
    }

  switch (playMode)
    {
  case SEQUENCE:
    pqSMAdaptor::setElementProperty(sceneProxy->GetProperty("NumberOfFrames"),
      dialogUI.spinBoxNumberOfFrames->value());
    break;

  case REALTIME:
    // Since even in real-time mode, while saving animation, it is played back 
    // in sequence mode, we change the NumberOfFrames instead of changing the
    // Duration. The spinBoxNumberOfFrames is updated to satisfy
    // duration * frame rate = number of frames.
    pqSMAdaptor::setElementProperty(sceneProxy->GetProperty("NumberOfFrames"),
      dialogUI.spinBoxNumberOfFrames->value());
    pqSMAdaptor::setElementProperty(sceneProxy->GetProperty("PlayMode"),
      SEQUENCE);
    break;

  case SNAP_TO_TIMESTEPS:
    pqSMAdaptor::setElementProperty(sceneProxy->GetProperty("FramesPerTimestep"),
      dialogUI.spinBoxFramesPerTimestep->value());
    break;
    }

  sceneProxy->UpdateVTKObjects();

  QSize newSize(dialogUI.width->text().toInt(),
                dialogUI.height->text().toInt());

  // Enforce any view size conditions (such a multiple of 4). 
  ::enforceMultiple4(newSize); 
  int magnification = viewManager?
    viewManager->prepareForCapture(newSize.width(), newSize.height()): 1;
 
  if (disconnect_and_save)
    {
    pqServer* server = this->Internals->ActiveServer;
    vtkSMSessionProxyManager* pxm = server->proxyManager();

    vtkSMProxy* writer = pxm->NewProxy("writers", "AnimationSceneImageWriter");
    pxm->RegisterProxy("animation", "writer", writer);
    vtkSMPropertyHelper(writer, "FileName").Set(filename.toAscii().data());
    vtkSMPropertyHelper(writer, "Magnification").Set(magnification);
    vtkSMPropertyHelper(writer, "FrameRate").Set(dialogUI.frameRate->value());
    writer->UpdateVTKObjects();
    writer->Delete();

     // Get ProxyManager XML state
    vtksys_ios::ostringstream xmlStringStream;
    vtkSmartPointer<vtkPVXMLElement> state;
    state.TakeReference(pxm->SaveXMLState());
    state->PrintXML(xmlStringStream, vtkIndent(0));

    // We create a server side proxy that will save the animation at disconnection.
    vtkSMProxy* cleaner = pxm->NewProxy("remote_player", "AnimationPlayer");
    vtkSMPropertyHelper(cleaner, "Writer").Set(writer);
    vtkSMPropertyHelper(cleaner, "XMLState").Set(xmlStringStream.str().c_str());
    cleaner->UpdateVTKObjects();
    pxm->RegisterProxy("animation","cleaner",cleaner);
    cleaner->Delete();

    // Make sure we delete all the view before disconnecting
    vtkNew<vtkSMProxyIterator> proxyIter;
    proxyIter->SetSession(server->session());
    std::vector<vtkSMViewProxy*> viewToDelete;
    for (proxyIter->Begin(); !proxyIter->IsAtEnd(); proxyIter->Next())
      {
      vtkSMViewProxy* view = vtkSMViewProxy::SafeDownCast(proxyIter->GetProxy());
      // We need to ensure that we skip prototypes.
      if (view)
        {
        viewToDelete.push_back(view);
        }
      }
    for(std::vector<vtkSMViewProxy*>::iterator it = viewToDelete.begin();
        it != viewToDelete.end(); it++)
      {
      pxm->UnRegisterProxy(*it);
      }

    // Disconnect from the server
    pqApplicationCore* core = pqApplicationCore::instance();
    if (server)
      {
      server->session()->PreDisconnection();
      core->getObjectBuilder()->removeServer(server);
      }

    return false;
    }

  // let the world know we are writing an animation.
  emit this->writeAnimation(filename, magnification, dialogUI.frameRate->value());

  vtkSMAnimationSceneImageWriter* writer = pqAnimationSceneImageWriter::New();
  writer->SetFileName(filename.toAscii().data());
  writer->SetMagnification(magnification);
  writer->SetAnimationScene(sceneProxy);
  writer->SetFrameRate(dialogUI.frameRate->value());

  pqProgressManager* progress_manager = 
    pqApplicationCore::instance()->getProgressManager();

  progress_manager->setEnableAbort(true);
  progress_manager->setEnableProgress(true);
  QObject::connect(progress_manager, SIGNAL(abort()), scene, SLOT(pause()));
  QObject::connect(scene, SIGNAL(tick(int)), this, SLOT(onTick(int)));
  QObject::connect(this, SIGNAL(saveProgress(const QString&, int)),
    progress_manager, SLOT(setProgress(const QString&, int)));
  progress_manager->lockProgress(this);
  bool status = writer->Save();
  progress_manager->unlockProgress(this);
  QObject::disconnect(progress_manager, 0, scene, 0);
  QObject::disconnect(scene, 0, this, 0);
  QObject::disconnect(this, 0, progress_manager, 0);
  progress_manager->setEnableProgress(false);
  progress_manager->setEnableAbort(false);
  writer->Delete();

  // Restore, duration and number of frames.
  switch (playMode)
    {
  case SEQUENCE:
    pqSMAdaptor::setElementProperty(sceneProxy->GetProperty("NumberOfFrames"), 
      num_frames);
    break;

  case REALTIME:
    pqSMAdaptor::setElementProperty(sceneProxy->GetProperty("NumberOfFrames"),
      num_frames);
    pqSMAdaptor::setElementProperty(sceneProxy->GetProperty("PlayMode"),
      REALTIME);
    break;

  case SNAP_TO_TIMESTEPS:
    pqSMAdaptor::setElementProperty(sceneProxy->GetProperty("FramesPerTimestep"),
      frames_per_timestep);
    break;
    }
  sceneProxy->UpdateVTKObjects();
  if (viewManager)
    {
    viewManager->cleanupAfterCapture();
    }

  if (stereo)
    {
    pqRenderViewBase::setStereo(0);
    }
  emit this->endNonUndoableChanges();
  return status;
}

//-----------------------------------------------------------------------------
bool pqAnimationManager::saveGeometry(const QString& filename, 
  pqView* view)
{
  if (!view)
    {
    return false;
    }

  pqAnimationScene* scene = this->getActiveScene();
  if (!scene)
    {
    return false;
    }
  vtkSMProxy* sceneProxy = scene->getProxy();
  vtkSMAnimationSceneGeometryWriter* writer = vtkSMAnimationSceneGeometryWriter::New();
  writer->SetFileName(filename.toAscii().data());
  writer->SetAnimationScene(sceneProxy);
  writer->SetViewModule(view->getProxy());
  bool status = writer->Save();
  writer->Delete();
  return status;
}

//-----------------------------------------------------------------------------
void pqAnimationManager::restoreSettings()
{
  // Load the most recently used file extension from QSettings, if available.
  pqSettings* settings = pqApplicationCore::instance()->settings();

  if ( settings->contains("extensions/AnimationExtension") )
    {
    this->AnimationExtension = settings->value("extensions/AnimationExtension").toString();
    }
  else
    {
    this->AnimationExtension = QString();
    }
}

//-----------------------------------------------------------------------------
void pqAnimationManager::saveSettings()
{ 
  // Save the most recently used file extension to QSettings.
  pqSettings* settings = pqApplicationCore::instance()->settings();
  settings->setValue("extensions/AnimationExtension", this->AnimationExtension);
}

//-----------------------------------------------------------------------------
void pqAnimationManager::updateApplicationSettings()
{
  foreach (QPointer<pqAnimationScene> scene, this->Internals->Scenes.values())
    {
    scene->updateApplicationSettings();
    }
}

//-----------------------------------------------------------------------------
void pqAnimationManager::onTick(int progress)
{
  emit this->saveProgress("Saving Animation", progress);
}
