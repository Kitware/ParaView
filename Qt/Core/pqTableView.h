/*=========================================================================

   Program: ParaView
   Module:    pqTableView.h

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

=========================================================================*/
#ifndef __pqTableView_h
#define __pqTableView_h

#include "pqView.h"

class PQCORE_EXPORT pqTableView :
  public pqView
{
  typedef pqView Superclass;

  Q_OBJECT
public:
  pqTableView(const QString& group, const QString& name, 
    vtkSMViewProxy* renModule, 
    pqServer* server, QObject* parent=NULL);
  ~pqTableView();

  static QString tableType() { return "TableView"; }
  static QString tableTypeName() { return "Table"; }

  QWidget* getWidget();

  /// Save a screenshot for the render module. If width or height ==0,
  /// the current window size is used.
  virtual bool saveImage(int /*width*/, int /*height*/, 
    const QString& /*filename*/) 
    {
    // Not supported yet.
    return false;
    };

  /// Save image to vtkImageData. Not supported.
  vtkImageData* captureImage(int /*magnification*/)
    { return NULL; }
  virtual vtkImageData* captureImage(const QSize& size)
    { return this->Superclass::captureImage(size); }

  /// Forces an immediate render. Overridden since for plots
  /// rendering actually happens on the GUI side, not merely
  /// in the ServerManager.
  virtual void forceRender();

  /// This method returns is any pqPipelineSource can be dislayed in this
  /// view. This is a convenience method, it gets
  /// the pqDisplayPolicy object from the pqApplicationCore
  /// are queries it.
  virtual bool canDisplay(pqOutputPort* opPort) const;

private slots:
  void visibilityChanged(pqRepresentation* disp);

private:
  pqTableView(const pqTableView&); // Not implemented.
  void operator=(const pqTableView&); // Not implemented.

  class pqImplementation;
  pqImplementation* const Implementation;
};


#endif

