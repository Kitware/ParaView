// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#include "pqLinkedObjectPythonTextArea.h"

//-----------------------------------------------------------------------------
template <>
inline pqTextLinker::pqTextLinker(pqPythonTextArea* t1, QTextEdit* t2)
  : Objs{ { std::unique_ptr<pqLinkedObjectInterface>(new pqLinkedObjectPythonTextArea(*t1)),
      std::unique_ptr<pqLinkedObjectInterface>(new pqLinkedObjectQTextEdit(*t2)) } }
{
  assert(t1);
  assert(t2);
}
