// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqTextLinker_h
#define pqTextLinker_h

#include "pqCoreModule.h"

#include "pqLinkedObjectInterface.h"

#include <QString>

#include <array>
#include <memory>

/**
 * @struct pqTextLinker
 * @brief Ease of use object that connects
 * two \ref pqLinkedObjectInterface together.
 * Note that the two objects are automatically
 * unlinked when this get destroyed.
 */
struct PQCORE_EXPORT pqTextLinker
{
  /**
   * @brief Default constructor with empty connections
   */
  pqTextLinker() noexcept = default;

  /**
   * @brief Templated constructors that takes abritrary
   * QTextEdit like objects
   */
  template <typename T, typename U>
  explicit pqTextLinker(T* /*t*/, U* /*u*/)
  {
    static_assert(sizeof(T) == 0 && sizeof(U) == 0,
      "Only specializations of pqTextLinker(T* t, U* u) can be used");
  }

  /**
   * @brief Copy assignement operators performs a deep
   * copy of the underlying data (only if they exist)
   */
  pqTextLinker& operator=(const pqTextLinker& other)
  {
    for (decltype(other.Objs)::size_type i = 0; i < other.Objs.size(); ++i)
    {
      if (other.Objs[i])
      {
        this->Objs[i] = other.Objs[i]->clone();
      }
      else
      {
        this->Objs[i] = nullptr;
      }
    }

    return *this;
  }

  /**
   * @brief Links both objects to each other
   */
  void link()
  {
    if (this->Objs[0] && this->Objs[1])
    {
      this->Objs[0]->link(this->Objs[1].get());
      this->Objs[1]->link(this->Objs[0].get());
    }
  }

  /**
   * @brief Unlinks both objects
   */
  void unlink()
  {
    for (auto const& obj : this->Objs)
    {
      if (obj)
      {
        obj->unlink();
      }
    }
  }

  /**
   * @brief Returns true if one object is linked to the other
   */
  bool isLinked() const noexcept
  {
    bool linked = false;
    for (auto const& obj : this->Objs)
    {
      if (!obj)
      {
        return false;
      }

      linked |= obj->isLinked();
    }

    return linked;
  }

  /**
   * @brief Returns true if the linked object is the given input parameter
   */
  bool isLinkedTo(const QObject* Obj) const
  {
    bool linked = false;
    for (auto const& obj : this->Objs)
    {
      if (!obj)
      {
        return false;
      }

      linked |= (Obj == obj->getLinked());
    }

    return linked;
  }

  /**
   * @brief Returns the first object name
   */
  QString getFirstObjectName() const
  {
    if (this->Objs[0])
    {
      return this->Objs[0]->getName();
    }

    return QString();
  }

  /**
   * @brief Returns the second object name
   */
  QString getSecondObjectName() const
  {
    if (this->Objs[1])
    {
      return this->Objs[1]->getName();
    }

    return QString();
  }

private:
  std::array<std::unique_ptr<pqLinkedObjectInterface>, 2> Objs = { { nullptr, nullptr } };
};

#include "pqTextLinker.txx"

#endif // pqTextLinker_h
