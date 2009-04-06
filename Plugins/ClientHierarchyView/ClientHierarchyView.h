/*
* Copyright (c) 2007, Sandia Corporation
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright
*       notice, this list of conditions and the following disclaimer in the
*       documentation and/or other materials provided with the distribution.
*     * Neither the name of the Sandia Corporation nor the
*       names of its contributors may be used to endorse or promote products
*       derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY Sandia Corporation ``AS IS'' AND ANY
* EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL Sandia Corporation BE LIABLE FOR ANY
* DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
* ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef _ClientHierarchyView_h
#define _ClientHierarchyView_h

#include <pqMultiInputView.h>

class vtkObject;
class ClientHierarchyView : public pqMultiInputView
{
  Q_OBJECT

public:
  ClientHierarchyView(
    const QString& viewtypemodule, 
    const QString& group, 
    const QString& name, 
    vtkSMViewProxy* viewmodule, 
    pqServer* server, 
    QObject* p);
  ~ClientHierarchyView();

  QWidget* getWidget();
  bool canDisplay(pqOutputPort* opPort) const;

  /// Capture the view image into a new vtkImageData with the given magnification
  /// and returns it.
  virtual vtkImageData* captureImage(int magnification);

  /// This view supports undo/redo.
  virtual bool supportsUndo() const { return true; }
 
  /// This view supports lookmarks.
  virtual bool supportsLookmarks() const { return true; }

protected:
  virtual void renderInternal();

private slots:
  /// Called whenever branches in the Qt tree widget are expanded/collapsed
  void treeVisibilityChanged();
  /// Called once to synchronize all views with changes in state / data
  void synchronizeViews();

  /// Called whenever the internal vtkView fires a ViewProgressEvent.
  void onViewProgressEvent(vtkObject* caller, 
    unsigned long vtk_event, void* client_data, void* call_data);

private:
  void showRepresentation(pqRepresentation*);
  void updateRepresentation(pqRepresentation*);
  void hideRepresentation(pqRepresentation*);
  void selectionChanged();

  void scheduleSynchronization(int update_flags);

  class implementation;
  implementation* const Implementation;
  class command;
  command* const Command;
};

#endif // _ClientHierarchyView_h

