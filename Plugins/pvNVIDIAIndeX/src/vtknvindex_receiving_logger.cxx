/* Copyright 2020 NVIDIA Corporation. All rights reserved.
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

#include "vtknvindex_receiving_logger.h"

#include <atomic>

#include "vtkOutputWindow.h"
#include "vtkSetGet.h"

#include <nv/index/version.h>

//----------------------------------------------------------------------
vtknvindex_receiving_logger::vtknvindex_receiving_logger()
{
  // empty
}

//----------------------------------------------------------------------
void vtknvindex_receiving_logger::message(mi::base::Message_severity level, const char* category,
  const mi::base::Message_details& /*details*/, const char* message)
{
#ifdef NDEBUG
  if (level >= mi::base::MESSAGE_SEVERITY_DEBUG) // No debug output in an optimized build.
    return;
#endif

  std::string message_str = message;

  if (message_str.empty() && level > mi::base::MESSAGE_SEVERITY_FATAL)
    return; // no message

  bool use_output_window = (level <= mi::base::MESSAGE_SEVERITY_WARNING);
  const std::string prefix = "nvindex: ";

  // Customize how some specific log messages are handled
  const std::string category_str = category;
  if (category_str == "TCPNET:NETWORK")
  {
    use_output_window = true; // Always show these in the Output Messages window

#if ((NVIDIA_INDEX_LIBRARY_REVISION_MAJOR > 327600 && NVIDIA_INDEX_LIBRARY_REVISION_MINOR > 0) ||  \
  NVIDIA_INDEX_LIBRARY_REVISION_MAJOR >= 337377)
    const std::string cluster_interface_warning =
      "TCPNET net  warn : The 'any' address can't be used as the cluster interface address.";
#else
    const std::string cluster_interface_warning =
      "TCPNET net  warn : The any address can't be used.";
#endif
    if (message_str.find(cluster_interface_warning) != std::string::npos)
    {
      level = mi::base::MESSAGE_SEVERITY_INFO; // make it less severe

      static bool already = false;
      if (!already)
      {
        already = true;
        message_str = std::string("Note: It is recommended to explicitly set the "
                                  "'cluster_interface_address' option. See the NVIDIA IndeX for "
                                  "ParaView Plugin User's Guide for details.\n") +
          prefix + message_str;
      }
    }
  }
  else if (category_str == "API:MISC")
  {
    // These messages are printed by DiCE during normal startup, skip them
    static std::atomic_int counter(0);
    if (counter == 0 && message_str.rfind("  0.0   API    misc info : Loaded \"", 0) == 0)
    {
      counter++;
      return;
    }
    else if (counter == 1 && message_str.rfind("  0.0   API    misc info : DiCE ", 0) == 0)
    {
      counter++;
      return;
    }
  }

  // This is based on vtkErrorWithObjectMacro()
  if (use_output_window && vtkObject::GetGlobalWarningDisplay())
  {
    // This will pop up the Output Messages window. The messages will also be printed to the
    // console.
    vtkOStreamWrapper::EndlType endl;
    vtkOStreamWrapper::UseEndl(endl);
    vtkOStrStreamWrapper vtkmsg;
    vtkmsg << prefix << message_str << '\n';

    if (level <= mi::base::MESSAGE_SEVERITY_ERROR)
    {
      vtkOutputWindowDisplayErrorText(vtkmsg.str());
    }
    else if (level <= mi::base::MESSAGE_SEVERITY_WARNING)
    {
      vtkOutputWindowDisplayWarningText(vtkmsg.str());
    }
    else
    {
      vtkOutputWindowDisplayText(vtkmsg.str());
    }

    vtkmsg.rdbuf()->freeze(0);
    if (level <= mi::base::MESSAGE_SEVERITY_WARNING)
    {
      vtkObject::BreakOnError();
    }
  }
  else
  {
    // Log only to console.
    std::cout << prefix << message_str << std::endl;
  }
}
