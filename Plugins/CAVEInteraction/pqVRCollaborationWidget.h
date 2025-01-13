// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqVRCollaborationWidget_h
#define pqVRCollaborationWidget_h

#include "vtkSMProxyLocator.h"

#include <QWidget>

class pqView;
class vtkPVXMLElement;
class vtkSMProxyLocator;

class pqVRCollaborationWidget : public QWidget
{
  Q_OBJECT
  typedef QWidget Superclass;

public:
  pqVRCollaborationWidget(QWidget* parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags{});
  virtual ~pqVRCollaborationWidget();

  void initializeCollaboration(pqView* view);
  void stopCollaboration();

public Q_SLOTS:
  void saveCollaborationState(vtkPVXMLElement*);
  void restoreCollaborationState(vtkPVXMLElement*, vtkSMProxyLocator*);

protected Q_SLOTS:
  void updateCollabWidgetState();

private Q_SLOTS:
  void configureAvatar();
  void collabEnabledChanged(Qt::CheckState state);
  void collabPortChanged();
  void collabServerChanged();
  void collabSessionChanged();
  void collabNameChanged();
  void collabAvatarUpXChanged();
  void collabAvatarUpYChanged();
  void collabAvatarUpZChanged();

private:
  Q_DISABLE_COPY(pqVRCollaborationWidget)

  class pqInternals;
  pqInternals* Internals;
};

#endif
