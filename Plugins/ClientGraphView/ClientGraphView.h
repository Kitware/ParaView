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

#ifndef _ClientGraphView_h
#define _ClientGraphView_h

#include <pqSingleInputView.h>
class vtkObject;

class ClientGraphView : public pqSingleInputView
{
  Q_OBJECT

public:
  ClientGraphView(
    const QString& viewtypemodule, 
    const QString& group, 
    const QString& name, 
    vtkSMViewProxy* viewmodule, 
    pqServer* server, 
    QObject* p);
  ~ClientGraphView();

  QWidget* getWidget();

  bool canDisplay(pqOutputPort* opPort) const;

  /// Capture the view image into a new vtkImageData with the given magnification
  /// and returns it.
  virtual vtkImageData* captureImage(int magnification);

  /// Save a screenshot for the view. If width or height ==0,
  /// the current window size is used.
  bool saveImage(int, int, const QString& );

  /// Returns if this view module can support 
  /// undo/redo. Returns false by default. Subclassess must override
  /// if that's not the case.
  virtual bool supportsUndo() const { return true; }
 
  /// This view supports lookmarks.
  virtual bool supportsLookmarks() const { return true; }

protected:
  virtual void renderInternal();

private slots:
  void onViewProgressEvent(vtkObject*,
    unsigned long vtk_event, void*, void* call_data);

private:
  void showRepresentation(pqRepresentation*);
  void hideRepresentation(pqRepresentation*);
  void selectionChanged();

  class implementation;
  implementation* const Implementation;

  class command;
  command* const Command;
};

#endif // _ClientGraphView_h

