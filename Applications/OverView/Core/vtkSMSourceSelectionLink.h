/*=========================================================================

   Program: ParaView
   Module:    vtkSMSourceSelectionLink.h

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
// .NAME vtkSMSourceSelectionLink - Links vtkSMSourceProxy selections
// .SECTION Description

#ifndef __vtkSMSourceSelectionLink_h
#define __vtkSMSourceSelectionLink_h

#include "OverViewCoreExport.h"

#include <vtkObject.h>

class vtkSMSourceProxy;
class vtkSMSourceSelectionLinkCommand;
class vtkSMSourceSelectionLinkInternals;
class pqViewManager;

class OVERVIEW_CORE_EXPORT vtkSMSourceSelectionLink : public vtkObject
{
public:
  static vtkSMSourceSelectionLink *New();
  vtkTypeMacro(vtkSMSourceSelectionLink, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  void AddSource(vtkSMSourceProxy* source);
  void RemoveSource(vtkSMSourceProxy* source);

  void SetViewManager(pqViewManager* manager)
    { this->ViewManager = manager; }

protected:
  vtkSMSourceSelectionLink();
  ~vtkSMSourceSelectionLink();

  void SelectionChanged(vtkSMSourceProxy* source);

private:
  vtkSMSourceSelectionLink(const vtkSMSourceSelectionLink&);  // Not implemented.
  void operator=(const vtkSMSourceSelectionLink&);  // Not implemented.

  pqViewManager* ViewManager;
  bool InSelectionChanged;

  friend class vtkSMSourceSelectionLinkCommand;
  vtkSMSourceSelectionLinkCommand* Command;
  vtkSMSourceSelectionLinkInternals* Internals;
};

#endif

