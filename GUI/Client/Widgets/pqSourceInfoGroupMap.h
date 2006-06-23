/*=========================================================================

   Program:   ParaView
   Module:    pqSourceInfoGroupMap.h

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

========================================================================*/

/// \file pqSourceInfoGroupMap.h
/// \date 5/31/2006

#ifndef _pqSourceInfoGroupMap_h
#define _pqSourceInfoGroupMap_h


#include "pqWidgetsExport.h"
#include <QObject>

class pqSourceInfoGroupMapItem;
class pqSourceInfoModel;
class QString;
class QStringList;
class vtkPVXMLElement;


class PQWIDGETS_EXPORT pqSourceInfoGroupMap : public QObject
{
  Q_OBJECT

public:
  pqSourceInfoGroupMap(QObject *parent=0);
  ~pqSourceInfoGroupMap();

  void loadSourceInfo(vtkPVXMLElement *root);
  void saveSourceInfo(vtkPVXMLElement *root);

  void addGroup(const QString &group);
  void removeGroup(const QString &group);

  void addSource(const QString &name, const QString &group);
  void removeSource(const QString &name, const QString &group);

  /// \brief
  ///   Adds all the source groups from the map to the model.
  /// \param model The model to initialize using the map.
  void initializeModel(pqSourceInfoModel *model) const;

signals:
  void clearingData();

  void groupAdded(const QString &group);
  void removingGroup(const QString &group);

  void sourceAdded(const QString &name, const QString &group);
  void removingSource(const QString &name, const QString &group);

private:
  pqSourceInfoGroupMapItem *getNextItem(pqSourceInfoGroupMapItem *item) const;
  void getGroupPath(pqSourceInfoGroupMapItem *item, QString &group) const;
  pqSourceInfoGroupMapItem *getGroupItemFor(const QString &group) const;

  pqSourceInfoGroupMapItem *getChildItem(pqSourceInfoGroupMapItem *item,
      const QString &name) const;
  bool isNameInItem(const QString &name, pqSourceInfoGroupMapItem *item) const;

private:
  pqSourceInfoGroupMapItem *Root;
};

#endif
