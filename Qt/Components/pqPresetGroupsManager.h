/*=========================================================================

   Program: ParaView
   Module:  pqPresetGroupManager.h

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
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

#ifndef pqPresetGroupManager_h
#define pqPresetGroupManager_h

#include "pqComponentsModule.h" // for exports

#include <QList>
#include <QMap>
#include <QObject>

class PQCOMPONENTS_EXPORT pqPresetGroupsManager : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  pqPresetGroupsManager(QObject* p);
  ~pqPresetGroupsManager();

  int numberOfGroups();
  int numberOfPresetsInGroup(const QString& groupName);
  int presetRankInGroup(const QString& presetName, const QString& groupName);
  QList<QString> groupNames();
  QString groupName(int i);

  void loadGroups(const QString& json);
  void clearGroups();

signals:
  void groupsUpdated();

private:
  QList<QString> GroupNames;
  QMap<QString, QList<QString> > Groups;
};

#endif
