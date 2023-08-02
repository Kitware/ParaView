// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

//-----------------------------------------------------------------------------
template <>
inline void pqPythonTextArea::linkTo<QTextEdit>(QTextEdit* obj)
{
  this->TextLinker = pqTextLinker(this, obj);
  this->TextLinker.link();
}
