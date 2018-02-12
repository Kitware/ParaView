/* Copyright 2018 NVIDIA Corporation. All rights reserved.
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

#ifndef __vtknvindex_utilities_h
#define __vtknvindex_utilities_h

#include <assert.h>
#include <fstream>
#include <sstream>

#ifdef WIN32
#else // WIN32
#include <pwd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#endif // WIN32

#include <fcntl.h>
#include <vector>

namespace vtknvindex
{
namespace util
{

//-------------------------------------------------------------------------------------------------
// Helper macro. Checks whether the expression is true and if not prints a message and exits.
#define check_success(expr)                                                                        \
  {                                                                                                \
    if (!(expr))                                                                                   \
    {                                                                                              \
      fprintf(stderr, "Error in file %s, line %d: \"%s\".\n", __FILE__, __LINE__, #expr);          \
      exit(EXIT_FAILURE);                                                                          \
    }                                                                                              \
  }

inline void sleep(mi::Float32 seconds)
{
#ifdef _WIN32
  // The windows version expects milliseconds here.
  mi::Uint32 millis = (mi::Uint32)(seconds * 1000);
  if (millis == 0)
    millis = 1;
  ::Sleep(millis);
#else
  useconds_t micros = (useconds_t)(seconds * 1000000);
  ::usleep((useconds_t)micros);
#endif // _WIN32
}

//-------------------------------------------------------------------------------------------------
// Get current time.
// \return get Float64 epoch time in second.
inline mi::Float64 get_time()
{
#ifdef WIN32
  static bool init = false;
  static mi::Float64 frequency;
  if (!init)
  {
    //_tzset();
    LARGE_INTEGER freq;
    QueryPerformanceFrequency(&freq);
    frequency = (mi::Float64)freq.QuadPart;
    init = true;
  }
  LARGE_INTEGER counter;
  QueryPerformanceCounter(&counter);

  return (mi::Float64)counter.QuadPart / frequency;
#else
  timeval tv;
  gettimeofday(&tv, NULL);
  return static_cast<mi::Float64>(tv.tv_sec) + (static_cast<mi::Float64>(tv.tv_usec) * 1.0e-6);
#endif
}

//----------------------------------------------------------------------
// Retrieves the host name of this machine.
// \return Returns the host name of this machine.
inline std::string get_host_name()
{
  std::string host_name = "unknown";
#ifdef LINUX
  char buf[256];
  if (gethostname(buf, sizeof(buf)) == 0)
    host_name = buf;
#else
  char* host_name_env = getenv("HOSTNAME");
  if (host_name_env == NULL)
    host_name_env = getenv("HOST");

  if (host_name_env != NULL)
    host_name = host_name_env;
// else
//    INFO_LOG << "Environment variable 'HOSTNAME' or 'HOST' not set on host.";
#endif // LINUX

  return host_name;
}

#ifndef WIN32
//----------------------------------------------------------------------
// Retrieves the process user name.
// \return Returns the process user name.
inline const char* get_process_user_name()
{
  uid_t uid = geteuid();
  struct passwd* pw = getpwuid(uid);
  if (pw)
  {
    return pw->pw_name;
  }

  return "";
}
#endif // WIN32

//-------------------------------------------------------------------------------------------------
// Return shared memory pointer to given name of the shared memory of size shared_seg_size
// After finishing using this shared memory pointer unmap_shm() must be call.
template <typename T>
inline T* get_vol_shm(const std::string& shmname, const mi::Uint64& shared_seg_size)
{

#ifdef WIN32
  HANDLE hMapFile;

  hMapFile = CreateFileMapping(
    INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, shared_seg_size, shmname.c_str());

  if (hMapFile == NULL)
  {
    ERROR_LOG << "shm_open(): Could not create file mapping object, shared memory: " << shmname
              << ".";
    return NULL;
  }

  T* shm_volume = (T*)MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, shared_seg_size);

  if (shm_volume == NULL)
  {
    CloseHandle(hMapFile);

    ERROR_LOG << "shm_open(): Could not map view of file, shared memory: " << shmname << ".";
    return NULL;
  }

  CloseHandle(hMapFile);

  return shm_volume;
#else

  mi::Sint32 shmfd = shm_open(shmname.c_str(), O_CREAT | O_RDWR, S_IRWXU);
  if (shmfd < 0)
  {
    ERROR_LOG << "Error shm_open() in vtknvindex_representation, shmname: " << shmname;
    return NULL;
  }

  // Adjusting mapped file size.
  if (ftruncate(shmfd, shared_seg_size) == -1)
  {
    ERROR_LOG << "The function ftruncate() failed to truncate the shared memory when writing, "
                 "shared memory: "
              << shmname << ".";
    return NULL;
  }

  T* shm_volume = (T*)mmap(NULL, shared_seg_size, PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0);
  if (shm_volume == NULL)
  {
    ERROR_LOG << "The function mmap() failed in vtknvindex_representation, shared memory: "
              << shmname << ".";
    return NULL;
  }
  close(shmfd);

  return shm_volume;
#endif
}

// Unmap shared memory from memory address space
inline void unmap_shm(void* shm_ptr, const mi::Uint64 shared_seg_size)
{
#ifdef WIN32
  UnmapViewOfFile(shm_ptr);
#else
  munmap(shm_ptr, shared_seg_size);
#endif
}

//-------------------------------------------------------------------------------------------------
inline mi::math::Color_struct convert_to_color_st(const mi::math::Color& col)
{
  mi::math::Color_struct col_st = {
    col.r, col.g, col.b, col.a,
  };
  return col_st;
}

//-------------------------------------------------------------------------------------------------
inline std::vector<mi::math::Color_struct> create_synth_colormap()
{

  std::vector<mi::math::Color> colormap_entry;
  colormap_entry.push_back(mi::math::Color(0, 0, 1, 0));
  colormap_entry.push_back(mi::math::Color(0, 0.00392157, 1, 0));
  colormap_entry.push_back(mi::math::Color(0, 0.00784314, 1, 0));
  colormap_entry.push_back(mi::math::Color(0, 0.0117647, 1, 0));
  colormap_entry.push_back(mi::math::Color(0, 0.0196078, 1, 0));
  colormap_entry.push_back(mi::math::Color(0, 0.0235294, 1, 0));
  colormap_entry.push_back(mi::math::Color(0, 0.027451, 1, 0));
  colormap_entry.push_back(mi::math::Color(0, 0.0313726, 1, 0));
  colormap_entry.push_back(mi::math::Color(0, 0.0352941, 1, 0));
  colormap_entry.push_back(mi::math::Color(0, 0.0392157, 1, 0));
  colormap_entry.push_back(mi::math::Color(0, 0.0431373, 1, 0));
  colormap_entry.push_back(mi::math::Color(0, 0.0470588, 1, 0));
  colormap_entry.push_back(mi::math::Color(0, 0.054902, 1, 0));
  colormap_entry.push_back(mi::math::Color(0, 0.0588235, 1, 0));
  colormap_entry.push_back(mi::math::Color(0, 0.0627451, 1, 0));
  colormap_entry.push_back(mi::math::Color(0, 0.0666667, 1, 0));
  colormap_entry.push_back(mi::math::Color(0, 0.0705882, 1, 0));
  colormap_entry.push_back(mi::math::Color(0, 0.0745098, 1, 0));
  colormap_entry.push_back(mi::math::Color(0, 0.0784314, 1, 0));
  colormap_entry.push_back(mi::math::Color(0, 0.0823529, 1, 0));
  colormap_entry.push_back(mi::math::Color(0, 0.0901961, 1, 0));
  colormap_entry.push_back(mi::math::Color(0, 0.0941176, 1, 0));
  colormap_entry.push_back(mi::math::Color(0, 0.0980392, 1, 0));
  colormap_entry.push_back(mi::math::Color(0, 0.101961, 1, 0));
  colormap_entry.push_back(mi::math::Color(0, 0.105882, 1, 0));
  colormap_entry.push_back(mi::math::Color(0, 0.109804, 1, 0));
  colormap_entry.push_back(mi::math::Color(0, 0.113725, 1, 0));
  colormap_entry.push_back(mi::math::Color(0, 0.121569, 1, 0));
  colormap_entry.push_back(mi::math::Color(0, 0.12549, 1, 0));
  colormap_entry.push_back(mi::math::Color(0, 0.129412, 1, 0));
  colormap_entry.push_back(mi::math::Color(0, 0.133333, 1, 0));
  colormap_entry.push_back(mi::math::Color(0, 0.137255, 1, 0));
  colormap_entry.push_back(mi::math::Color(0, 0.141176, 1, 0));
  colormap_entry.push_back(mi::math::Color(0, 0.145098, 1, 0));
  colormap_entry.push_back(mi::math::Color(0, 0.14902, 1, 0));
  colormap_entry.push_back(mi::math::Color(0, 0.156863, 1, 0));
  colormap_entry.push_back(mi::math::Color(0, 0.160784, 1, 0));
  colormap_entry.push_back(mi::math::Color(0, 0.164706, 1, 0));
  colormap_entry.push_back(mi::math::Color(0, 0.168627, 1, 0));
  colormap_entry.push_back(mi::math::Color(0, 0.172549, 1, 0));
  colormap_entry.push_back(mi::math::Color(0, 0.176471, 1, 0));
  colormap_entry.push_back(mi::math::Color(0, 0.180392, 1, 0));
  colormap_entry.push_back(mi::math::Color(0, 0.184314, 1, 0));
  colormap_entry.push_back(mi::math::Color(0, 0.192157, 1, 0));
  colormap_entry.push_back(mi::math::Color(0, 0.196078, 1, 0));
  colormap_entry.push_back(mi::math::Color(0, 0.2, 1, 0));
  colormap_entry.push_back(mi::math::Color(0, 0.203922, 1, 0));
  colormap_entry.push_back(mi::math::Color(0, 0.207843, 1, 0));
  colormap_entry.push_back(mi::math::Color(0, 0.211765, 1, 0));
  colormap_entry.push_back(mi::math::Color(0, 0.215686, 1, 0));
  colormap_entry.push_back(mi::math::Color(0, 0.223529, 1, 0));
  colormap_entry.push_back(mi::math::Color(0, 0.227451, 1, 0));
  colormap_entry.push_back(mi::math::Color(0, 0.231373, 1, 0));
  colormap_entry.push_back(mi::math::Color(0, 0.235294, 1, 0));
  colormap_entry.push_back(mi::math::Color(0, 0.239216, 1, 0));
  colormap_entry.push_back(mi::math::Color(0, 0.243137, 1, 0));
  colormap_entry.push_back(mi::math::Color(0, 0.247059, 1, 0));
  colormap_entry.push_back(mi::math::Color(0, 0.25098, 1, 0));
  colormap_entry.push_back(mi::math::Color(0, 0.258824, 1, 0));
  colormap_entry.push_back(mi::math::Color(0, 0.262745, 1, 0));
  colormap_entry.push_back(mi::math::Color(0, 0.266667, 1, 0.000351434));
  colormap_entry.push_back(mi::math::Color(0, 0.270588, 1, 0.00197421));
  colormap_entry.push_back(mi::math::Color(0, 0.27451, 1, 0.00359698));
  colormap_entry.push_back(mi::math::Color(0, 0.278431, 1, 0.00521975));
  colormap_entry.push_back(mi::math::Color(0, 0.286275, 1, 0.00684252));
  colormap_entry.push_back(mi::math::Color(0, 0.290196, 1, 0.0084653));
  colormap_entry.push_back(mi::math::Color(0, 0.294118, 1, 0.0100881));
  colormap_entry.push_back(mi::math::Color(0, 0.298039, 1, 0.0117108));
  colormap_entry.push_back(mi::math::Color(0, 0.301961, 1, 0.0133336));
  colormap_entry.push_back(mi::math::Color(0, 0.305882, 1, 0.0149564));
  colormap_entry.push_back(mi::math::Color(0, 0.313726, 1, 0.0165792));
  colormap_entry.push_back(mi::math::Color(0, 0.317647, 1, 0.0182019));
  colormap_entry.push_back(mi::math::Color(0, 0.321569, 1, 0.0198247));
  colormap_entry.push_back(mi::math::Color(0, 0.32549, 1, 0.0214475));
  colormap_entry.push_back(mi::math::Color(0, 0.329412, 1, 0.0230702));
  colormap_entry.push_back(mi::math::Color(0, 0.333333, 1, 0.024693));
  colormap_entry.push_back(mi::math::Color(0, 0.341176, 1, 0.0263158));
  colormap_entry.push_back(mi::math::Color(0, 0.345098, 1, 0.0263158));
  colormap_entry.push_back(mi::math::Color(0, 0.34902, 1, 0.0263158));
  colormap_entry.push_back(mi::math::Color(0, 0.352941, 1, 0.0263158));
  colormap_entry.push_back(mi::math::Color(0, 0.356863, 1, 0.0263158));
  colormap_entry.push_back(mi::math::Color(0, 0.360784, 1, 0.0263158));
  colormap_entry.push_back(mi::math::Color(0, 0.368627, 1, 0.0263158));
  colormap_entry.push_back(mi::math::Color(0, 0.372549, 1, 0.0263158));
  colormap_entry.push_back(mi::math::Color(0, 0.376471, 1, 0.0263158));
  colormap_entry.push_back(mi::math::Color(0, 0.380392, 1, 0.0263158));
  colormap_entry.push_back(mi::math::Color(0, 0.384314, 1, 0.0263158));
  colormap_entry.push_back(mi::math::Color(0, 0.392157, 1, 0.0263158));
  colormap_entry.push_back(mi::math::Color(0, 0.396078, 1, 0.0263158));
  colormap_entry.push_back(mi::math::Color(0, 0.4, 1, 0.0263158));
  colormap_entry.push_back(mi::math::Color(0, 0.403922, 1, 0.0263158));
  colormap_entry.push_back(mi::math::Color(0, 0.407843, 1, 0.0263158));
  colormap_entry.push_back(mi::math::Color(0, 0.411765, 1, 0.0263158));
  colormap_entry.push_back(mi::math::Color(0, 0.419608, 1, 0.0263158));
  colormap_entry.push_back(mi::math::Color(0, 0.423529, 1, 0.0263158));
  colormap_entry.push_back(mi::math::Color(0, 0.427451, 1, 0.0263158));
  colormap_entry.push_back(mi::math::Color(0, 0.431373, 1, 0.0263158));
  colormap_entry.push_back(mi::math::Color(0, 0.435294, 1, 0.0263158));
  colormap_entry.push_back(mi::math::Color(0, 0.439216, 1, 0.0263158));
  colormap_entry.push_back(mi::math::Color(0, 0.447059, 1, 0.0263158));
  colormap_entry.push_back(mi::math::Color(0, 0.45098, 1, 0.0263158));
  colormap_entry.push_back(mi::math::Color(0, 0.454902, 1, 0.0263158));
  colormap_entry.push_back(mi::math::Color(0, 0.458824, 1, 0.0263158));
  colormap_entry.push_back(mi::math::Color(0, 0.462745, 1, 0.0263158));
  colormap_entry.push_back(mi::math::Color(0, 0.466667, 1, 0.0263158));
  colormap_entry.push_back(mi::math::Color(0, 0.47451, 1, 0.0263158));
  colormap_entry.push_back(mi::math::Color(0, 0.478431, 1, 0.0263158));
  colormap_entry.push_back(mi::math::Color(0, 0.482353, 1, 0.0263158));
  colormap_entry.push_back(mi::math::Color(0, 0.486275, 1, 0.0263158));
  colormap_entry.push_back(mi::math::Color(0, 0.490196, 1, 0.0263158));
  colormap_entry.push_back(mi::math::Color(0, 0.498039, 1, 0.0263158));
  colormap_entry.push_back(mi::math::Color(0, 0.501961, 1, 0.0263158));
  colormap_entry.push_back(mi::math::Color(0, 0.505882, 1, 0.0263158));
  colormap_entry.push_back(mi::math::Color(0, 0.509804, 1, 0.0263158));
  colormap_entry.push_back(mi::math::Color(0, 0.513726, 1, 0.0263158));
  colormap_entry.push_back(mi::math::Color(0, 0.517647, 1, 0.0263158));
  colormap_entry.push_back(mi::math::Color(0, 0.52549, 1, 0.0263158));
  colormap_entry.push_back(mi::math::Color(0, 0.529412, 1, 0.0263158));
  colormap_entry.push_back(mi::math::Color(0, 0.533333, 1, 0.0263158));
  colormap_entry.push_back(mi::math::Color(0, 0.537255, 1, 0.0263158));
  colormap_entry.push_back(mi::math::Color(0, 0.541176, 1, 0.0263158));
  colormap_entry.push_back(mi::math::Color(0, 0.545098, 1, 0.0263158));
  colormap_entry.push_back(mi::math::Color(0, 0.552941, 1, 0.0263158));
  colormap_entry.push_back(mi::math::Color(0, 0.556863, 1, 0.0263158));
  colormap_entry.push_back(mi::math::Color(0, 0.560784, 1, 0.0263158));
  colormap_entry.push_back(mi::math::Color(0, 0.564706, 1, 0.0263158));
  colormap_entry.push_back(mi::math::Color(0, 0.568627, 1, 0.0263158));
  colormap_entry.push_back(mi::math::Color(0, 0.572549, 1, 0.0263158));
  colormap_entry.push_back(mi::math::Color(0, 0.580392, 1, 0.0263158));
  colormap_entry.push_back(mi::math::Color(0, 0.603922, 1, 0.0263158));
  colormap_entry.push_back(mi::math::Color(0, 0.639216, 1, 0.0263158));
  colormap_entry.push_back(mi::math::Color(0, 0.67451, 1, 0.0263158));
  colormap_entry.push_back(mi::math::Color(0, 0.709804, 1, 0.0263158));
  colormap_entry.push_back(mi::math::Color(0, 0.745098, 1, 0.0263158));
  colormap_entry.push_back(mi::math::Color(0, 0.780392, 1, 0.0263158));
  colormap_entry.push_back(mi::math::Color(0, 0.811765, 1, 0.0263158));
  colormap_entry.push_back(mi::math::Color(0, 0.847059, 1, 0.0263158));
  colormap_entry.push_back(mi::math::Color(0, 0.882353, 1, 0.0263158));
  colormap_entry.push_back(mi::math::Color(0, 0.917647, 1, 0.0263158));
  colormap_entry.push_back(mi::math::Color(0, 0.952941, 1, 0.0263158));
  colormap_entry.push_back(mi::math::Color(0, 0.988235, 1, 0.0263158));
  colormap_entry.push_back(mi::math::Color(0, 1, 0.976471, 0.0263158));
  colormap_entry.push_back(mi::math::Color(0, 1, 0.941176, 0.0263158));
  colormap_entry.push_back(mi::math::Color(0, 1, 0.905882, 0.0263158));
  colormap_entry.push_back(mi::math::Color(0, 1, 0.87451, 0.0263158));
  colormap_entry.push_back(mi::math::Color(0, 1, 0.839216, 0.0263158));
  colormap_entry.push_back(mi::math::Color(0, 1, 0.803922, 0.0263158));
  colormap_entry.push_back(mi::math::Color(0, 1, 0.768627, 0.0263158));
  colormap_entry.push_back(mi::math::Color(0, 1, 0.733333, 0.0263158));
  colormap_entry.push_back(mi::math::Color(0, 1, 0.698039, 0.0263158));
  colormap_entry.push_back(mi::math::Color(0, 1, 0.662745, 0.0263158));
  colormap_entry.push_back(mi::math::Color(0, 1, 0.627451, 0.0263158));
  colormap_entry.push_back(mi::math::Color(0, 1, 0.592157, 0.0263158));
  colormap_entry.push_back(mi::math::Color(0, 1, 0.560784, 0.0263158));
  colormap_entry.push_back(mi::math::Color(0, 1, 0.52549, 0.0263158));
  colormap_entry.push_back(mi::math::Color(0, 1, 0.490196, 0.0263158));
  colormap_entry.push_back(mi::math::Color(0, 1, 0.454902, 0.0263158));
  colormap_entry.push_back(mi::math::Color(0, 1, 0.419608, 0.0263158));
  colormap_entry.push_back(mi::math::Color(0, 1, 0.384314, 0.0263158));
  colormap_entry.push_back(mi::math::Color(0, 1, 0.34902, 0.0263158));
  colormap_entry.push_back(mi::math::Color(0, 1, 0.313726, 0.0263158));
  colormap_entry.push_back(mi::math::Color(0, 1, 0.278431, 0.0263158));
  colormap_entry.push_back(mi::math::Color(0, 1, 0.231373, 0.0263158));
  colormap_entry.push_back(mi::math::Color(0, 1, 0.164706, 0.0263158));
  colormap_entry.push_back(mi::math::Color(0, 1, 0.0980392, 0.0263158));
  colormap_entry.push_back(mi::math::Color(0, 1, 0.0313726, 0.0263158));
  colormap_entry.push_back(mi::math::Color(0.0352941, 1, 0, 0.0263158));
  colormap_entry.push_back(mi::math::Color(0.0980392, 1, 0, 0.0263158));
  colormap_entry.push_back(mi::math::Color(0.164706, 1, 0, 0.0263158));
  colormap_entry.push_back(mi::math::Color(0.231373, 1, 0, 0.0263158));
  colormap_entry.push_back(mi::math::Color(0.298039, 1, 0, 0.0263158));
  colormap_entry.push_back(mi::math::Color(0.364706, 1, 0, 0.0263158));
  colormap_entry.push_back(mi::math::Color(0.431373, 1, 0, 0.0263158));
  colormap_entry.push_back(mi::math::Color(0.498039, 1, 0, 0.0263158));
  colormap_entry.push_back(mi::math::Color(0.564706, 1, 0, 0.0263158));
  colormap_entry.push_back(mi::math::Color(0.631373, 1, 0, 0.0263158));
  colormap_entry.push_back(mi::math::Color(0.698039, 1, 0, 0.0263158));
  colormap_entry.push_back(mi::math::Color(0.764706, 1, 0, 0.0263158));
  colormap_entry.push_back(mi::math::Color(0.831373, 1, 0, 0.0263158));
  colormap_entry.push_back(mi::math::Color(0.898039, 1, 0, 0.0263158));
  colormap_entry.push_back(mi::math::Color(0.964706, 1, 0, 0.0263158));
  colormap_entry.push_back(mi::math::Color(1, 0.968627, 0, 0.0263158));
  colormap_entry.push_back(mi::math::Color(1, 0.901961, 0, 0.0263158));
  colormap_entry.push_back(mi::math::Color(1, 0.835294, 0, 0.0263158));
  colormap_entry.push_back(mi::math::Color(1, 0.768627, 0, 0.0263158));
  colormap_entry.push_back(mi::math::Color(1, 0.701961, 0, 0.0263158));
  colormap_entry.push_back(mi::math::Color(1, 0.635294, 0, 0.0263158));
  colormap_entry.push_back(mi::math::Color(1, 0.568627, 0, 0.0263158));
  colormap_entry.push_back(mi::math::Color(1, 0.52549, 0, 0.0263158));
  colormap_entry.push_back(mi::math::Color(1, 0.517647, 0, 0.0263158));
  colormap_entry.push_back(mi::math::Color(1, 0.509804, 0, 0.0263158));
  colormap_entry.push_back(mi::math::Color(1, 0.501961, 0, 0.0263158));
  colormap_entry.push_back(mi::math::Color(1, 0.494118, 0, 0.0263158));
  colormap_entry.push_back(mi::math::Color(1, 0.486275, 0, 0.0263158));
  colormap_entry.push_back(mi::math::Color(1, 0.478431, 0, 0.0263158));
  colormap_entry.push_back(mi::math::Color(1, 0.470588, 0, 0.0263158));
  colormap_entry.push_back(mi::math::Color(1, 0.462745, 0, 0.0263158));
  colormap_entry.push_back(mi::math::Color(1, 0.454902, 0, 0.0263158));
  colormap_entry.push_back(mi::math::Color(1, 0.447059, 0, 0.0263158));
  colormap_entry.push_back(mi::math::Color(1, 0.439216, 0, 0.0263158));
  colormap_entry.push_back(mi::math::Color(1, 0.431373, 0, 0.0263158));
  colormap_entry.push_back(mi::math::Color(1, 0.423529, 0, 0.0263158));
  colormap_entry.push_back(mi::math::Color(1, 0.415686, 0, 0.0263158));
  colormap_entry.push_back(mi::math::Color(1, 0.407843, 0, 0.0263158));
  colormap_entry.push_back(mi::math::Color(1, 0.4, 0, 0.0263158));
  colormap_entry.push_back(mi::math::Color(1, 0.392157, 0, 0.0263158));
  colormap_entry.push_back(mi::math::Color(1, 0.384314, 0, 0.0263158));
  colormap_entry.push_back(mi::math::Color(1, 0.376471, 0, 0.0263158));
  colormap_entry.push_back(mi::math::Color(1, 0.368627, 0, 0.0263158));
  colormap_entry.push_back(mi::math::Color(1, 0.360784, 0, 0.0263158));
  colormap_entry.push_back(mi::math::Color(1, 0.352941, 0, 0.0263158));
  colormap_entry.push_back(mi::math::Color(1, 0.345098, 0, 0.0263158));
  colormap_entry.push_back(mi::math::Color(1, 0.337255, 0, 0.0263158));
  colormap_entry.push_back(mi::math::Color(1, 0.329412, 0, 0.0263158));
  colormap_entry.push_back(mi::math::Color(1, 0.321569, 0, 0.0263158));
  colormap_entry.push_back(mi::math::Color(1, 0.313726, 0, 0.0263158));
  colormap_entry.push_back(mi::math::Color(1, 0.305882, 0, 0.0263158));
  colormap_entry.push_back(mi::math::Color(1, 0.298039, 0, 0.0263158));
  colormap_entry.push_back(mi::math::Color(1, 0.290196, 0, 0.0263158));
  colormap_entry.push_back(mi::math::Color(1, 0.282353, 0, 0.0263158));
  colormap_entry.push_back(mi::math::Color(1, 0.27451, 0, 0.0263158));
  colormap_entry.push_back(mi::math::Color(1, 0.266667, 0, 0.0263158));
  colormap_entry.push_back(mi::math::Color(1, 0.258824, 0, 0.0263158));
  colormap_entry.push_back(mi::math::Color(1, 0.25098, 0, 0.0263158));
  colormap_entry.push_back(mi::math::Color(1, 0.243137, 0, 0.0263158));
  colormap_entry.push_back(mi::math::Color(1, 0.235294, 0, 0.0263158));
  colormap_entry.push_back(mi::math::Color(1, 0.227451, 0, 0.0263158));
  colormap_entry.push_back(mi::math::Color(1, 0.219608, 0, 0.0263158));
  colormap_entry.push_back(mi::math::Color(1, 0.211765, 0, 0.0263158));
  colormap_entry.push_back(mi::math::Color(1, 0.203922, 0, 0.0263158));
  colormap_entry.push_back(mi::math::Color(1, 0.196078, 0, 0.0263158));
  colormap_entry.push_back(mi::math::Color(1, 0.188235, 0, 0.0263158));
  colormap_entry.push_back(mi::math::Color(1, 0.180392, 0, 0.0263158));
  colormap_entry.push_back(mi::math::Color(1, 0.172549, 0, 0.0263158));
  colormap_entry.push_back(mi::math::Color(1, 0.164706, 0, 0.0263158));
  colormap_entry.push_back(mi::math::Color(1, 0.156863, 0, 0.0263158));
  colormap_entry.push_back(mi::math::Color(1, 0.14902, 0, 0.0263158));
  colormap_entry.push_back(mi::math::Color(1, 0.141176, 0, 0.0263158));
  colormap_entry.push_back(mi::math::Color(1, 0.133333, 0, 0.0263158));
  colormap_entry.push_back(mi::math::Color(1, 0.12549, 0, 0.0263158));
  colormap_entry.push_back(mi::math::Color(1, 0.117647, 0, 0.0263158));
  colormap_entry.push_back(mi::math::Color(1, 0.109804, 0, 0.0263158));
  colormap_entry.push_back(mi::math::Color(1, 0.101961, 0, 0.0263158));
  colormap_entry.push_back(mi::math::Color(1, 0.0941176, 0, 0.0263158));
  colormap_entry.push_back(mi::math::Color(1, 0.0862745, 0, 0.0263158));
  colormap_entry.push_back(mi::math::Color(1, 0.0784314, 0, 0.0263158));
  colormap_entry.push_back(mi::math::Color(1, 0.0705882, 0, 0.0263158));
  colormap_entry.push_back(mi::math::Color(1, 0.0627451, 0, 0.0263158));
  colormap_entry.push_back(mi::math::Color(1, 0.054902, 0, 0.0263158));
  colormap_entry.push_back(mi::math::Color(1, 0.0470588, 0, 0.0263158));
  colormap_entry.push_back(mi::math::Color(1, 0.0392157, 0, 0.0263158));
  colormap_entry.push_back(mi::math::Color(1, 0.0313726, 0, 0.0263158));
  colormap_entry.push_back(mi::math::Color(1, 0.0235294, 0, 0.0263158));
  colormap_entry.push_back(mi::math::Color(1, 0.0156863, 0, 0.0263158));
  colormap_entry.push_back(mi::math::Color(1, 0.00784314, 0, 0.0263158));
  colormap_entry.push_back(mi::math::Color(1, 0, 0, 0.0263158));

  std::vector<mi::math::Color_struct> colormap_entry_st;
  mi::Size const colormap_count = colormap_entry.size();
  for (mi::Size i = 0; i < colormap_count; ++i)
  {
    colormap_entry_st.push_back(convert_to_color_st(colormap_entry.at(i)));
  }

  return colormap_entry_st;
}

//-------------------------------------------------------------------------------------------------
// String serialize utility.

inline void serialize(mi::neuraylib::ISerializer* serializer, const std::string& source)
{
  mi::Uint32 nb_elements = static_cast<mi::Uint32>(source.size());
  serializer->write(&nb_elements, 1);
  if (nb_elements > 0)
  {
    serializer->write(reinterpret_cast<const mi::Uint8*>(source.data()), nb_elements);
  }
}

inline void deserialize(mi::neuraylib::IDeserializer* deserializer, std::string& target)
{
  mi::Uint32 nb_elements = 0;
  deserializer->read(&nb_elements, 1);
  if (nb_elements > 0)
  {
    target.resize(nb_elements);
    deserializer->read(reinterpret_cast<mi::Uint8*>(&target[0]), nb_elements);
  }
  else
  {
    target.clear();
  }
}
}
} // vtknvindex::util

#endif
