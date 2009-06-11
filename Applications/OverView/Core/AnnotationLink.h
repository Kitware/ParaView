/*=========================================================================

   Program: ParaView
   Module:    AnnotationLink.h

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
#ifndef __AnnotationLink_h
#define __AnnotationLink_h

#include <QObject>
#include "OverViewCoreExport.h"

class vtkAnnotationLink;
class vtkSelection;
class AnnotationLinkCommand;
class AnnotationLinkInternals;
class pqPipelineSource;
class pqServer;
class pqView;
class vtkSMSourceProxy;

/// Provides a central location for managing annotations

class OVERVIEW_CORE_EXPORT AnnotationLink : public QObject
{
  Q_OBJECT
  
public:
  static AnnotationLink& instance();

  void initialize(pqServer* server);
  
  vtkAnnotationLink* getLink();
  vtkSMSourceProxy* getLinkProxy();

  void updateViews();

private slots:
  void onSourceAdded(pqPipelineSource*);
  void onSourceRemoved(pqPipelineSource*);
  void onViewCreated(pqView*);
  void onViewDestroyed(pqView*);

protected:
  void annotationsChanged(vtkSMSourceProxy* source);

private:
  AnnotationLink();
  AnnotationLink(const AnnotationLink&); // Not implemented.
  void operator=(const AnnotationLink&); // Not implemented.
  ~AnnotationLink();

  friend class AnnotationLinkCommand;
  AnnotationLinkCommand* Command;
  AnnotationLinkInternals* Internals;
};

#endif
