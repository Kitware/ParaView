/*=========================================================================

  Program:   ParaView
  Module:    pqPythonViewOptions.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef _pqPythonViewOptions_h
#define _pqPythonViewOptions_h

#include "pqOptionsContainer.h"
#include <QPointer>

class pqView;
class pqPythonView;

/// options container for pages of multislice view options
class PQCOMPONENTS_EXPORT pqPythonViewOptions : public pqOptionsContainer
{
  Q_OBJECT

public:
  pqPythonViewOptions(QWidget *parent=0);
  virtual ~pqPythonViewOptions();

  // set the view to show options for
  void setView(pqView* view);

  // set the current page
  virtual void setPage(const QString &page);
  // return a list of strings for pages we have
  virtual QStringList getPageList();

  // apply the changes
  virtual void applyChanges();
  // reset the changes
  virtual void resetChanges();

  // tell pqOptionsDialog that we want an apply button
  virtual bool isApplyUsed() const { return true; }

protected:
  QPointer<pqPythonView> View;

private:
  class pqInternal;
  pqInternal* Internal;
};


#endif
