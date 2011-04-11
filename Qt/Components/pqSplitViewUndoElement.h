/*=========================================================================

   Program: ParaView
   Module:    pqSplitViewUndoElement.h

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
#ifndef __pqSplitViewUndoElement_h
#define __pqSplitViewUndoElement_h

#include "vtkSMUndoElement.h"
#include "pqComponentsExport.h"
#include "pqMultiView.h" // needed for pqMultiView.

/// pqSplitViewUndoElement is an undo element for splitting of views.
/// pqViewManager creates an undo element on every split and pushes
/// it on the stack.
/// Make sure that the undo element is registered with the
/// state loader for the undo stack on which it is pushed. 
class PQCOMPONENTS_EXPORT pqSplitViewUndoElement : public vtkSMUndoElement
{
public:
  static pqSplitViewUndoElement* New();
  vtkTypeMacro(pqSplitViewUndoElement, vtkSMUndoElement);
  void PrintSelf(ostream& os, vtkIndent indent);

  /// Description:
  /// Undo the operation encapsulated by this element.
  virtual int Undo();

  /// Description:
  /// Redo the operation encaspsulated by this element.
  /// We make the pqProxy aware of its helper proxies.
  virtual int Redo();

  // Description:
  // Creates the undo element for the split operation.
  void SplitView(
    const pqMultiView::Index& index, Qt::Orientation orientation, 
    float fraction, const pqMultiView::Index& childIndex);

protected:
  pqSplitViewUndoElement();
  ~pqSplitViewUndoElement();

  int UndoInternal();
  int RedoInternal();

  vtkSetStringMacro(Index);
  vtkSetStringMacro(ChildIndex);

  char* Index;
  char* ChildIndex;
  int Orientation;
  float Percent;

private:
  pqSplitViewUndoElement(const pqSplitViewUndoElement&); // Not implemented.
  void operator=(const pqSplitViewUndoElement&); // Not implemented.
};


#endif

