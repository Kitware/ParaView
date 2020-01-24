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

#ifndef vtknvindex_forwarding_logger_h
#define vtknvindex_forwarding_logger_h

#include <sstream>
#include <string>

#include <mi/base/ilogger.h>
#include <mi/base/interface_implement.h>
#include <mi/dice.h>
#include <nv/index/iindex.h>

namespace vtknvindex
{
namespace logger
{

// The class vtknvindex_forwarding_logger forwards warning/errors messages gathered
// by the NVIDIA IndeX ParaView plugin to ParaView's console.

class vtknvindex_forwarding_logger
{
public:
  vtknvindex_forwarding_logger();
  virtual ~vtknvindex_forwarding_logger();

  // Get message stream.
  std::ostringstream& get_message(mi::base::Message_severity level);

  // Get the string representation of the severity level.
  static std::string level_to_string(mi::base::Message_severity level);

private:
  vtknvindex_forwarding_logger(const vtknvindex_forwarding_logger&) = delete;
  void operator=(const vtknvindex_forwarding_logger&) = delete;

  std::ostringstream m_os;                                 // Output stream.
  mi::base::Message_severity m_level;                      // Message severity level.
  mi::base::Handle<mi::base::ILogger> m_forwarding_logger; // Forwarding logger-
};

// The class vtknvindex_forwarding_logger represents a singleton and operates as a factory
// for message logging.
class vtknvindex_forwarding_logger_factory
{
public:
  vtknvindex_forwarding_logger_factory();
  virtual ~vtknvindex_forwarding_logger_factory();

  static vtknvindex_forwarding_logger_factory* instance()
  {
    if (G_p_forwarding_logger_factory == 0)
    {
      G_p_forwarding_logger_factory = new vtknvindex_forwarding_logger_factory();
    }
    return G_p_forwarding_logger_factory;
  }

  static void delete_instance()
  {
    if (G_p_forwarding_logger_factory != 0)
    {
      delete G_p_forwarding_logger_factory;
      G_p_forwarding_logger_factory = 0;
    }
  }

  // Initialize the factory, only once.
  void initialize(mi::base::Handle<nv::index::IIndex>& iindex_if);

  // Set a message header.
  void set_message_header(std::string const& header_str);

  std::string get_message_header() const;

  bool shutdown();

  // Returns true when this factory is ready to use.
  bool is_enabled() const;

  // Set fall back (when dice is not available) log severity.
  bool set_fallback_log_severity(mi::base::Message_severity fb_level);

  mi::base::Message_severity get_fallback_log_severity() const;

  mi::base::ILogger* get_forwarding_logger() const;

private:
  vtknvindex_forwarding_logger_factory(const vtknvindex_forwarding_logger_factory&) = delete;
  void operator=(const vtknvindex_forwarding_logger_factory&) = delete;

  static vtknvindex_forwarding_logger_factory* G_p_forwarding_logger_factory; // Singleton.

  std::string m_header_str;
  mi::base::Handle<nv::index::IIndex>
    m_iindex_if; // NVIDIA IndeX interface to access to the forwarding logger.
  mi::base::Message_severity m_fallback_severity_level; // Fall back message severity level.
};
}
} // namespace

//
// Overload the stream operator for some common types.
//

// To be able to call the operators from any other namespace, they need to be defined in the same
// namespace where their argument type is defined (argument-depended name lookup / Koenig lookup).
namespace mi
{
namespace math
{

/// Overloaded ostream for Vector<*,2>.
///     \param[in] str Represents the output stream.
///     \param[in] vec Represents the output vector.
///     \return Returns the output stream.
template <typename T>
inline std::ostream& operator<<(std::ostream& str, const mi::math::Vector<T, 2>& vec)
{
  return (str << "[" << vec.x << " " << vec.y << "]");
}

/// Overloaded ostream for Vector<*,3>.
///     \param[in] str Represents the output stream.
///     \param[in] vec Represents the output vector.
///     \return Returns the output stream.
template <typename T>
inline std::ostream& operator<<(std::ostream& str, const mi::math::Vector<T, 3>& vec)
{
  return (str << "[" << vec.x << " " << vec.y << " " << vec.z << "]");
}

/// Overloaded ostream for Vector<*,4>.
///     \param[in] str Represents the output stream.
///     \param[in] vec Represents the output vector.
///     \return Returns the output stream.
template <typename T>
inline std::ostream& operator<<(std::ostream& str, const mi::math::Vector<T, 4>& vec)
{
  return (str << "[" << vec.x << " " << vec.y << " " << vec.z << " " << vec.w << "]");
}

/// Overloaded ostream for color.
///     \param[in] str Represents the output stream.
///     \param[in] col Represents the output color.
///     \return Returns the output stream.
inline std::ostream& operator<<(std::ostream& str, const mi::math::Color& col)
{
  return (str << "[" << col.r << " " << col.g << " " << col.b << " " << col.a << "]");
}

/// overloaded ostream for Bbox<*,2>.
///     \param[in] str Represents the output stream.
///     \param[in] bbox Represents the output bounding box.
///     \return Returns the output stream.
template <typename T>
inline std::ostream& operator<<(std::ostream& str, const mi::math::Bbox<T, 2>& bbox)
{
  return (str << "[" << bbox.min.x << " " << bbox.min.y << "; " << bbox.max.x << " " << bbox.max.y
              << "]");
}

/// Overloaded ostream for Bbox<*,3>.
///     \param[in] str Represents the output stream.
///     \param[in] bbox Represents the output bounding box.
///     \return Returns the output stream.
template <typename T>
inline std::ostream& operator<<(std::ostream& str, const mi::math::Bbox<T, 3>& bbox)
{
  return (str << "[" << bbox.min.x << " " << bbox.min.y << " " << bbox.min.z << "; " << bbox.max.x
              << " " << bbox.max.y << " " << bbox.max.z << "]");
}

/// Overloaded ostream for Matrix<*,3,3>.
///     \param[in] str Represents the output stream.
///     \param[in] mat Represents the output matrix.
///     \return Returns the output stream.
template <typename T>
inline std::ostream& operator<<(std::ostream& str, const mi::math::Matrix<T, 3, 3>& mat)
{
  return (str << "\t[ " << mat.xx << "\t " << mat.yx << "\t " << mat.zx << "\n"
              << "\t  " << mat.xy << "\t " << mat.yy << "\t " << mat.zy << "\n"
              << "\t  " << mat.xz << "\t " << mat.yz << "\t " << mat.zz << "\t ]");
}

/// overloaded ostream for Matrix<*,4,4>.
///     \param[in] str Represents the output stream.
///     \param[in] mat Represents the output matrix.
///     \return Returns the output stream.
template <typename T>
inline std::ostream& operator<<(std::ostream& str, const mi::math::Matrix<T, 4, 4>& mat)
{
  return (str << "\t[ " << mat.xx << "\t " << mat.yx << "\t " << mat.zx << "\t " << mat.wx << "\n"
              << "\t  " << mat.xy << "\t " << mat.yy << "\t " << mat.zy << "\t " << mat.wy << "\n"
              << "\t  " << mat.xz << "\t " << mat.yz << "\t " << mat.zz << "\t " << mat.wz << "\n"
              << "\t  " << mat.xw << "\t " << mat.yw << "\t " << mat.zw << "\t " << mat.ww
              << "\t ]");
}

/// overloaded ostream for Vector_struct<*,2>.
///     \param[in] str Represents the output stream.
///     \param[in] vec Represents the output vector.
///     \return Returns the output stream.
template <typename T>
inline std::ostream& operator<<(std::ostream& str, const mi::math::Vector_struct<T, 2>& vec)
{
  return (str << mi::math::Vector<T, 2>(vec));
}

/// overloaded ostream for Vector_struct<*,3>.
///     \param[in] str Represents the output stream.
///     \param[in] vec Represents the output vector.
///     \return Returns the output stream.
template <typename T>
inline std::ostream& operator<<(std::ostream& str, const mi::math::Vector_struct<T, 3>& vec)
{
  return (str << mi::math::Vector<T, 3>(vec));
}

/// overloaded ostream for Vector_struct<*,4>.
///     \param[in] str Represents the output stream.
///     \param[in] vec Represents the output vector.
///     \return Returns the output stream.
template <typename T>
inline std::ostream& operator<<(std::ostream& str, const mi::math::Vector_struct<T, 4>& vec)
{
  return (str << mi::math::Vector<T, 4>(vec));
}

/// overloaded ostream for Color_struct.
///     \param[in] str Represents the output stream.
///     \param[in] col Represents the output color.
///     \return Returns the output stream.
inline std::ostream& operator<<(std::ostream& str, const mi::math::Color_struct& col)
{
  return (str << mi::math::Color(col));
}

/// overloaded ostream for Bbox_struct<*,2>.
///     \param[in] str Represents the output stream.
///     \param[in] bbox Represents the output vector.
///     \return Returns the output stream.
template <typename T>
inline std::ostream& operator<<(std::ostream& str, const mi::math::Bbox_struct<T, 2>& bbox)
{
  return (str << mi::math::Bbox<T, 2>(bbox));
}

/// overloaded ostream for Bbox_struct<*,3>.
///     \param[in] str Represents the output stream.
///     \param[in] bbox Represents the output bounding box.
///     \return Returns the output stream.
template <typename T>
inline std::ostream& operator<<(std::ostream& str, const mi::math::Bbox_struct<T, 3>& bbox)
{
  return (str << mi::math::Bbox<T, 3>(bbox));
}

/// overloaded ostream for Matrix_struct<*,3,3>.
///     \param[in] str Represents the output stream.
///     \param[in] mat Represents the output matrix.
///     \return Returns the output stream.
template <typename T>
inline std::ostream& operator<<(std::ostream& str, const mi::math::Matrix_struct<T, 3, 3>& mat)
{
  return (str << mi::math::Matrix<T, 3, 3>(mat));
}

/// overloaded ostream for Matrix_struct<*,4,4>.
///     \param[in] str Represents the output stream.
///     \param[in] mat Represents the output matrix.
///     \return Returns the output stream.
template <typename T>
inline std::ostream& operator<<(std::ostream& str, const mi::math::Matrix_struct<T, 4, 4>& mat)
{
  return (str << mi::math::Matrix<T, 4, 4>(mat));
}

} // namespace mi::math

namespace neuraylib
{

/// overloaded ostream for Tag.
///     \param[in] str Represents the output stream.
///     \param[in] bbox Represents the output tag.
///     \return Returns the output stream.
inline std::ostream& operator<<(std::ostream& str, const mi::neuraylib::Tag& tag)
{
  return (str << tag.id);
}

} // namespace mi::neuraylib
} // namespace mi

/// Only output message in debug builds.
#define DEBUG_LOG                                                                                  \
  vtknvindex::logger::vtknvindex_forwarding_logger().get_message(mi::base::MESSAGE_SEVERITY_DEBUG)
/// Log output stream for verbose information.
#define VERBOSE_LOG                                                                                \
  vtknvindex::logger::vtknvindex_forwarding_logger().get_message(mi::base::MESSAGE_SEVERITY_VERBOSE)
/// Log output stream for information.
#define INFO_LOG                                                                                   \
  vtknvindex::logger::vtknvindex_forwarding_logger().get_message(mi::base::MESSAGE_SEVERITY_INFO)
/// Log output stream for warning.
#define WARN_LOG                                                                                   \
  vtknvindex::logger::vtknvindex_forwarding_logger().get_message(mi::base::MESSAGE_SEVERITY_WARNING)
/// Log output stream for error.
#define ERROR_LOG                                                                                  \
  vtknvindex::logger::vtknvindex_forwarding_logger().get_message(mi::base::MESSAGE_SEVERITY_ERROR)

#endif // vtknvindex_forwarding_logger_h
