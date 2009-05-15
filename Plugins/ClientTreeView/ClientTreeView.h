/*-------------------------------------------------------------------------
  Copyright 2009 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#ifndef _ClientTreeView_h
#define _ClientTreeView_h

#include <pqSingleInputView.h>

class QItemSelection;
class vtkSelection;

class ClientTreeView : public pqSingleInputView
{
  Q_OBJECT

public:
  ClientTreeView(
    const QString& viewtypemodule, 
    const QString& group, 
    const QString& name, 
    vtkSMViewProxy* viewmodule, 
    pqServer* server, 
    QObject* p);
  ~ClientTreeView();

  QWidget* getWidget();

  bool canDisplay(pqOutputPort* opPort) const;

protected:
  /// Implement this to perform the actual rendering.
  virtual void renderInternal();

private:
  void showRepresentation(pqRepresentation*);
  void hideRepresentation(pqRepresentation*);
  void updateRepresentation(pqRepresentation*);

  void selectionChanged();

  class implementation;
  implementation* const Implementation;

  class command;
  command* const Command;
};

#endif // _ClientTreeView_h

