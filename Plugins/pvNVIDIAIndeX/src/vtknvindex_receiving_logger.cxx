/* Copyright 2019 NVIDIA Corporation. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions
* are met:
*  * Redistributions of source code must retain the above copyright
*    notice, this list of conditions and the following disclaimer.
*  * Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the distribution.
*  * Neither the name of NVIDIA CORPORATION nor the names of its
*    contributors may be used to endorse or promote products derived
*    from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
* EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
* PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
* CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
* EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
* PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
* OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/// \file

#include "vtknvindex_receiving_logger.h"

#include <iostream>

#include "vtkOutputWindow.h"
#include "vtkSetGet.h"

namespace vtknvindex
{
namespace logger
{

//----------------------------------------------------------------------
vtknvindex_receiving_logger::vtknvindex_receiving_logger()
  : m_os()
  , m_level(mi::base::MESSAGE_SEVERITY_INFO)
{
  // empty
}

//----------------------------------------------------------------------
vtknvindex_receiving_logger::~vtknvindex_receiving_logger()
{
#ifdef NDEBUG
  if (m_level >= mi::base::MESSAGE_SEVERITY_DEBUG) // No debug output in an optimized build.
    return;
#endif

  if (m_os.str().empty() && m_level > mi::base::MESSAGE_SEVERITY_FATAL)
    return; // no message

  const std::string prefix = "nvindex: ";

  // This is based on vtkErrorWithObjectMacro()
  if (m_level <= mi::base::MESSAGE_SEVERITY_WARNING && vtkObject::GetGlobalWarningDisplay())
  {
    // This will pop up the Output Messages window. The messages will also be printed to the
    // console.
    vtkOStreamWrapper::EndlType endl;
    vtkOStreamWrapper::UseEndl(endl);
    vtkOStrStreamWrapper vtkmsg;
    vtkmsg << prefix << m_os.str() << '\n';

    if (m_level <= mi::base::MESSAGE_SEVERITY_ERROR)
    {
      vtkOutputWindowDisplayErrorText(vtkmsg.str());
    }
    else
    {
      vtkOutputWindowDisplayWarningText(vtkmsg.str());
    }

    vtkmsg.rdbuf()->freeze(0);
    vtkObject::BreakOnError();
  }
  else
  {
    // Log info level only to console.
    std::cout << prefix << m_os.str() << std::endl;
  }
}

//----------------------------------------------------------------------
std::ostringstream& vtknvindex_receiving_logger::get_message(mi::Uint32 level)
{
  m_level = level;
  return m_os;
}

//----------------------------------------------------------------------
void vtknvindex_receiving_logger::message(
  mi::base::Message_severity level, const char* /*category*/, const char* message)
{
  vtknvindex_receiving_logger().get_message(level) << message;
}
}
} // namespace
