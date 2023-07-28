// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqDisplayPanelPropertyWidget_h
#define pqDisplayPanelPropertyWidget_h

#include <pqPropertyWidget.h>

#include "pqDisplayPanel.h"

class pqDisplayPanelPropertyWidget : public pqPropertyWidget
{
  Q_OBJECT

public:
  explicit pqDisplayPanelPropertyWidget(pqDisplayPanel* displayPanel, QWidget* parent = nullptr);
  ~pqDisplayPanelPropertyWidget() override;

  pqDisplayPanel* getDisplayPanel() const;

private:
  pqDisplayPanel* DisplayPanel;
};

#endif // pqDisplayPanelPropertyWidget_h
