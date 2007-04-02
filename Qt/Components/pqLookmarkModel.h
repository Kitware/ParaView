/*=========================================================================

   Program: ParaView
   Module:    pqLookmarkModel.h

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.1. 

   See License_v1.1.txt for the full ParaView license.
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

/// \file pqLookmarkModel.h

#ifndef _pqLookmarkModel_h
#define _pqLookmarkModel_h

#include "pqComponentsExport.h"
#include <QObject>
#include <QImage>
#include "vtkSmartPointer.h"

class vtkSMStateLoader;
class pqServer;
class pqGenericViewModule;
class vtkPVXMLElement;
class pqPipelineSource;

/// \class pqLookmarkModel
/// \brief
///   The pqLookmarkModel class stores the metadata of a lookmark including: name, 
///   server manager state, description, an icon, a preview of the pipeline, and flags that control how it is loaded.
/// 
///   Lookmarks can be saved to and initialized from XML. The XML representation of a lookmark is as follows:
///
///     <LookmarkDefinition name="My Lookmark" Comments="Here are a few thoughts..." RestoreData="1" RestoreCamera="1">
///         <Icon value="KDJFLSKDJFLJDLSKFJLDSKJFLSDJFLSDKJFLKDSJFLSDKFLSL.../>
///         <Pipeline value="KDJFLSKDJFLJDLSKFJLDSKJFLSDJFLSDKJFLKDSJFLSDKFLSL.../>
///         <ServerManagerState>
///           ....
///         </ServerManagerState>
///     </LookmarkDefinition>

class PQCOMPONENTS_EXPORT pqLookmarkModel : public QObject
{
  Q_OBJECT

public:

  // Only a name and server manager state are required to create a lookmark
  //pqLookmarkModel(QString name, QString state, QObject* parent=NULL);
  pqLookmarkModel(QString name, const QString &state, QObject* parent=NULL);
  pqLookmarkModel(const pqLookmarkModel &other, QObject* parent=NULL);
  // Alternatively, a lookmark can be initialized from a <LookmarkDefinition> XML element
  pqLookmarkModel(vtkPVXMLElement *lmkState, QObject* parent=NULL);
  virtual ~pqLookmarkModel(){};

  // Access Methods:

  // The name of a lookmark is unique among all lookmarks in the application
  QString getName()const {return this->Name;};

  // Get the server manager state stored as a qstring
  QString getState() const;
  
  // When this flag is set, the state of any readers and root sources of the pipeline will be loaded, 
  // otherwise the state loader will try to use existing ones
  bool getRestoreDataFlag()const {return this->RestoreData;};

  // When this flag is set, the camera state in this lookmark's server manager state will override paraview's current camera,
  // Otherwise the current camera properties will remain unchanged when this lookmark is loaded.
  bool getRestoreCameraFlag()const {return this->RestoreCamera;};

  bool getRestoreTimeFlag()const {return this->RestoreTime;};

  // User-defined text can be stored along with the lookmark
  QString getDescription()const {return this->Description;};

  // (Optional) screenshot of the view(s) when the lookmark was created
  const QImage& getIcon()const {return this->Icon;};

  // (Optional) snapshot of the pipeline at the time the lookmark was created
  //const QImage& getPipelinePreview(){return this->Pipeline;};
  //QString getPipelineHierarchy(){return this->Pipeline;};
  vtkPVXMLElement* getPipelineHierarchy() const;

  // Save the lookmark's data to the given lookmark element
  void saveState(vtkPVXMLElement *lookmarkElement) const;

  // allow lookmarks of lookmarks? if this lookmark is a multi-view, return lookmarks for individual views?
  // QList<pqLookmarkModel *> getLookmarkItems();

public slots:

  // For multi-view, do we remove current pqGenericViewModules before loading?
  // Is there a case where the user would just want to load the lookmark state without specifying a view to display it in?

  // Display this lookmark in the given view, on the given server, using the given state loader
  // Setting the default view only makes sense if this lookmark is made up of a single view because
  // otherwise existing views will not be reused
  virtual void load(pqServer *server,QList<pqPipelineSource*> *sources, pqGenericViewModule *view=NULL, vtkSMStateLoader *loader=NULL);

  // The name of a lookmark is unique among all lookmarks in the application
  void setName(QString name);

  // Set the server manager state stored as an xml tree
  void setState(QString state);

  // When this flag is set, the state of any readers and root sources of the pipeline will be loaded, 
  // otherwise the state loader will try to use existing ones
  void setRestoreDataFlag(bool state);

  // When this flag is set, the camera state in this lookmark's server manager state will override paraview's current camera,
  // Otherwise the current camera properties will remain unchanged when this lookmark is loaded.
  void setRestoreCameraFlag(bool state);

  // When this is set, the time value of the view stored in 
  //  the lookmark overrides the current one when loaded
  void setRestoreTimeFlag(bool state);

  // User-defined text can be stored along with the lookmark
  void setDescription(QString text);

  // (Optional) screenshot of the view(s) when the lookmark was created
  void setIcon(const QImage icon);

  // (Optional) snapshot of the pipeline at the time the lookmark was created
  void setPipelineHierarchy(vtkPVXMLElement *pipeline);

signals:
  void modified(pqLookmarkModel*);
  void nameChanged(const QString &oldName, const QString &newName);
  void loaded(pqLookmarkModel*);

private:
  void initializeState(vtkPVXMLElement *state);

  QString Name;
  QString State;
  bool RestoreData;
  bool RestoreCamera;
  bool RestoreTime;
  QString Description;
  QImage Icon;
  vtkSmartPointer<vtkPVXMLElement> PipelineHierarchy;
};

#endif
