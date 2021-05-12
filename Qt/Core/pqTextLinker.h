/*=========================================================================

   Program: ParaView
   Module:    pqTextLinker.h

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
  explicit pqTextLinker(T* t, U* u)
  {
    static_assert(sizeof(T) == 0 && sizeof(U) == 0,
      "Only specializations of pqTextLinker(T* t, U* u) can be used");
  }

  /**
   * @brief Copy assignement operators performs a deep
   * copy of the underlying data (only if they exist)
   */
  pqTextLinker& operator=(const pqTextLinker& other) noexcept
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
