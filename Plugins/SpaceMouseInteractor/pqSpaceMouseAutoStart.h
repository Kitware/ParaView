// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqSpaceMouseAutoStart_h
#define pqSpaceMouseAutoStart_h

#include <QObject>

class pqSpaceMouseImpl;
class pqView;

class pqSpaceMouseAutoStart : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  pqSpaceMouseAutoStart(QObject* parent = nullptr);
  ~pqSpaceMouseAutoStart() override;

  void startup();
  void shutdown();

  // public Q_SLOTS:
  // void setActiveView(pqView* view);

private:
  Q_DISABLE_COPY(pqSpaceMouseAutoStart)

  pqSpaceMouseImpl* m_p;
};

#endif
