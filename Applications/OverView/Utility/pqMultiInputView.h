/*=========================================================================

   Program: ParaView
   Module:    pqMultiInputView.h

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

#ifndef _pqMultiInputView_h
#define _pqMultiInputView_h

#include "OverViewUtilityExport.h"

#include "pqView.h"

/// Convenience view class that enforces "single input" behavior
class OVERVIEW_UTILITY_EXPORT pqMultiInputView : public pqView
{
  Q_OBJECT

public:
  pqMultiInputView(
    const QString& viewtypemodule, 
    const QString& group, 
    const QString& name, 
    vtkSMViewProxy* viewmodule, 
    pqServer* server, 
    QObject* p);
  ~pqMultiInputView();

  /// Provide a do-nothing implementation for this method.
  bool saveImage(int, int, const QString& );
  /// Provide a do-nothing implementation for this method.
  vtkImageData* captureImage(int);
  vtkImageData* captureImage(const QSize& asize)
    { return this->pqView::captureImage(asize); }

protected:

private slots:
  void onRepresentationAdded(pqRepresentation*);
  void onRepresentationVisibilityChanged(bool visible);
  void onRepresentationUpdated();
  void onRepresentationRemoved(pqRepresentation*);
  void onStateLoaded();

  /// Implement this in derived classes to display the given representation
  virtual void showRepresentation(pqRepresentation*) = 0;
  /// Implement this in derived classes to update the display because the representation has changed
  virtual void updateRepresentation(pqRepresentation*) = 0;
  /// Implement this in derived classes to hide the given representation
  virtual void hideRepresentation(pqRepresentation*) = 0;

private:
  class implementation;
  implementation* const Implementation;
};

#endif // _pqMultiInputView_h

