/*=========================================================================

   Program: ParaView
   Module:    pqOptionsContainer.h

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

/// \file pqOptionsContainer.h
/// \date 7/20/2007

#ifndef _pqOptionsContainer_h
#define _pqOptionsContainer_h


#include "pqComponentsModule.h"
#include "pqOptionsPage.h"


/// \class pqOptionsContainer
/// \brief
///   The pqOptionsContainer class is used to add multiple pages of
///   options to the pqOptionsDialog.
///
/// Grouping the options pages into container objects can make is
/// easier to maintain a set of options. The container makes it
/// possible to reuse a UI form. If several objects have the same
/// properties, the same page can be used for each of the objects.
class PQCOMPONENTS_EXPORT pqOptionsContainer : public pqOptionsPage
{
  Q_OBJECT

public:
  /// \brief
  ///   Constructs an options container.
  /// \param parent The parent widget.
  pqOptionsContainer(QWidget *parent=0);
  virtual ~pqOptionsContainer();

  /// \brief
  ///   Gets the page path prefix.
  /// \return
  ///   The page path prefix.
  const QString &getPagePrefix() const;

  /// \brief
  ///   Sets the page path prefix.
  /// \param prefix The new page path prefix.
  void setPagePrefix(const QString &prefix);

  /// \brief
  ///   Sets the currently displayed page.
  /// \param page The page hierarchy name.
  virtual void setPage(const QString &page) = 0;
  
  /// \brief
  ///   Gets the list of available pages in the container.
  /// \param pages Used to return the list of available pages.
  virtual QStringList getPageList() = 0;

private:
  QString *Prefix; ///< Stores the page prefix.
};

#endif

