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

#ifndef vtknvindex_receiving_logger_h
#define vtknvindex_receiving_logger_h

#include <sstream>

#include <mi/base/ilogger.h>
#include <mi/dice.h>

namespace vtknvindex
{
namespace logger
{

// The class vtknvindex_forwarding_logger forwards warning/errors messages gathered
// by the NVIDIA IndeX library to ParaView's console.

class vtknvindex_receiving_logger : public mi::base::Interface_implement<mi::base::ILogger>
{
public:
  vtknvindex_receiving_logger();
  virtual ~vtknvindex_receiving_logger();

  /// Get message stream.
  /// \param[in] level message The severity level of the message.
  std::ostringstream& get_message(mi::Uint32 level);

  // Set message and severity values.
  void message(
    mi::base::Message_severity level, const char* category, const char* message) override;

private:
  // Output stream.
  std::ostringstream m_os;
  // Message severity level.
  mi::Uint32 m_level;

  vtknvindex_receiving_logger(const vtknvindex_receiving_logger&) = delete;
  void operator=(const vtknvindex_receiving_logger&) = delete;
};
}
} // namespace

#endif // vtknvindex_receiving_logger_h
