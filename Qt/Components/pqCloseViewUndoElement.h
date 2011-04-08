/*=========================================================================

   Program: ParaView
   Module:    pqCloseViewUndoElement.h

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

========================================================================*/
#ifndef __pqCloseViewUndoElement_h 
#define __pqCloseViewUndoElement_h

#include "vtkSMUndoElement.h"
#include "pqComponentsExport.h"
#include "pqMultiView.h"
#include <vtkSmartPointer.h>

class vtkSMCacheBasedProxyLocator;

/// pqCloseViewUndoElement is undo element used to undo the closing
/// of a view frame.
/// pqViewManager creates an undo element on every frame close and pushes
/// it on the stack.
/// Make sure that the undo element is registered with the
/// state loader for the undo stack on which it is pushed. 
class VTK_EXPORT pqCloseViewUndoElement : public vtkSMUndoElement 
{
public:
  static pqCloseViewUndoElement* New();
  vtkTypeMacro(pqCloseViewUndoElement, vtkSMUndoElement);
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
  // \c invert flag inverts the operation of this undo element. When true,
  // Undo() does with Redo() would when invert=false, and vice-versa.
  void CloseView(pqMultiView::Index frameIndex, vtkPVXMLElement* state);

  // Description:
  // Get CacheBasedProxyLocator in order to register proxy that we are
  // interessted in.
  vtkGetObjectMacro(ViewStateCache, vtkSMCacheBasedProxyLocator);

protected:
  pqCloseViewUndoElement();
  ~pqCloseViewUndoElement();

  vtkSetStringMacro(Index);
  char* Index;
  vtkSmartPointer<vtkPVXMLElement> State;
  vtkSMCacheBasedProxyLocator* ViewStateCache;

private:
  pqCloseViewUndoElement(const pqCloseViewUndoElement&); // Not implemented.
  void operator=(const pqCloseViewUndoElement&); // Not implemented.
};

#endif


