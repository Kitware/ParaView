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

#ifndef vtknvindex_utilities_h
#define vtknvindex_utilities_h

#include <assert.h>
#include <fstream>
#include <sstream>

#ifdef _WIN32
#else // _WIN32
#include <pwd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#endif // _WIN32

#include <fcntl.h>
#include <vector>

namespace vtknvindex
{
namespace util
{

// some typedefs
typedef mi::math::Vector<mi::Sint32, 3> Vec3i;
typedef mi::math::Vector<mi::Uint32, 3> Vec3u;

typedef mi::math::Vector_struct<mi::Sint32, 3> Vec3i_struct;
typedef mi::math::Vector_struct<mi::Uint32, 3> Vec3u_struct;

typedef mi::math::Bbox<mi::Sint32, 3> Bbox3i;
typedef mi::math::Bbox<mi::Uint32, 3> Bbox3u;

typedef mi::math::Bbox_struct<mi::Sint32, 3> Bbox3i_struct;
typedef mi::math::Bbox_struct<mi::Uint32, 3> Bbox3u_struct;

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
#ifdef _WIN32
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

#ifndef _WIN32
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
#endif // _WIN32

//-------------------------------------------------------------------------------------------------
// Return shared memory pointer to given name of the shared memory of size shared_seg_size
// After finishing using this shared memory pointer unmap_shm() must be call.
template <typename T>
inline T* get_vol_shm(const std::string& shmname, const mi::Uint64& shared_seg_size)
{

#ifdef _WIN32
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
#ifdef _WIN32
  UnmapViewOfFile(shm_ptr);
  (void)shared_seg_size;
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
  colormap_entry.push_back(mi::math::Color(0, 0.00392157f, 1, 0));
  colormap_entry.push_back(mi::math::Color(0, 0.00784314f, 1, 0));
  colormap_entry.push_back(mi::math::Color(0, 0.0117647f, 1, 0));
  colormap_entry.push_back(mi::math::Color(0, 0.0196078f, 1, 0));
  colormap_entry.push_back(mi::math::Color(0, 0.0235294f, 1, 0));
  colormap_entry.push_back(mi::math::Color(0, 0.027451f, 1, 0));
  colormap_entry.push_back(mi::math::Color(0, 0.0313726f, 1, 0));
  colormap_entry.push_back(mi::math::Color(0, 0.0352941f, 1, 0));
  colormap_entry.push_back(mi::math::Color(0, 0.0392157f, 1, 0));
  colormap_entry.push_back(mi::math::Color(0, 0.0431373f, 1, 0));
  colormap_entry.push_back(mi::math::Color(0, 0.0470588f, 1, 0));
  colormap_entry.push_back(mi::math::Color(0, 0.054902f, 1, 0));
  colormap_entry.push_back(mi::math::Color(0, 0.0588235f, 1, 0));
  colormap_entry.push_back(mi::math::Color(0, 0.0627451f, 1, 0));
  colormap_entry.push_back(mi::math::Color(0, 0.0666667f, 1, 0));
  colormap_entry.push_back(mi::math::Color(0, 0.0705882f, 1, 0));
  colormap_entry.push_back(mi::math::Color(0, 0.0745098f, 1, 0));
  colormap_entry.push_back(mi::math::Color(0, 0.0784314f, 1, 0));
  colormap_entry.push_back(mi::math::Color(0, 0.0823529f, 1, 0));
  colormap_entry.push_back(mi::math::Color(0, 0.0901961f, 1, 0));
  colormap_entry.push_back(mi::math::Color(0, 0.0941176f, 1, 0));
  colormap_entry.push_back(mi::math::Color(0, 0.0980392f, 1, 0));
  colormap_entry.push_back(mi::math::Color(0, 0.101961f, 1, 0));
  colormap_entry.push_back(mi::math::Color(0, 0.105882f, 1, 0));
  colormap_entry.push_back(mi::math::Color(0, 0.109804f, 1, 0));
  colormap_entry.push_back(mi::math::Color(0, 0.113725f, 1, 0));
  colormap_entry.push_back(mi::math::Color(0, 0.121569f, 1, 0));
  colormap_entry.push_back(mi::math::Color(0, 0.12549f, 1, 0));
  colormap_entry.push_back(mi::math::Color(0, 0.129412f, 1, 0));
  colormap_entry.push_back(mi::math::Color(0, 0.133333f, 1, 0));
  colormap_entry.push_back(mi::math::Color(0, 0.137255f, 1, 0));
  colormap_entry.push_back(mi::math::Color(0, 0.141176f, 1, 0));
  colormap_entry.push_back(mi::math::Color(0, 0.145098f, 1, 0));
  colormap_entry.push_back(mi::math::Color(0, 0.14902f, 1, 0));
  colormap_entry.push_back(mi::math::Color(0, 0.156863f, 1, 0));
  colormap_entry.push_back(mi::math::Color(0, 0.160784f, 1, 0));
  colormap_entry.push_back(mi::math::Color(0, 0.164706f, 1, 0));
  colormap_entry.push_back(mi::math::Color(0, 0.168627f, 1, 0));
  colormap_entry.push_back(mi::math::Color(0, 0.172549f, 1, 0));
  colormap_entry.push_back(mi::math::Color(0, 0.176471f, 1, 0));
  colormap_entry.push_back(mi::math::Color(0, 0.180392f, 1, 0));
  colormap_entry.push_back(mi::math::Color(0, 0.184314f, 1, 0));
  colormap_entry.push_back(mi::math::Color(0, 0.192157f, 1, 0));
  colormap_entry.push_back(mi::math::Color(0, 0.196078f, 1, 0));
  colormap_entry.push_back(mi::math::Color(0, 0.2f, 1, 0));
  colormap_entry.push_back(mi::math::Color(0, 0.203922f, 1, 0));
  colormap_entry.push_back(mi::math::Color(0, 0.207843f, 1, 0));
  colormap_entry.push_back(mi::math::Color(0, 0.211765f, 1, 0));
  colormap_entry.push_back(mi::math::Color(0, 0.215686f, 1, 0));
  colormap_entry.push_back(mi::math::Color(0, 0.223529f, 1, 0));
  colormap_entry.push_back(mi::math::Color(0, 0.227451f, 1, 0));
  colormap_entry.push_back(mi::math::Color(0, 0.231373f, 1, 0));
  colormap_entry.push_back(mi::math::Color(0, 0.235294f, 1, 0));
  colormap_entry.push_back(mi::math::Color(0, 0.239216f, 1, 0));
  colormap_entry.push_back(mi::math::Color(0, 0.243137f, 1, 0));
  colormap_entry.push_back(mi::math::Color(0, 0.247059f, 1, 0));
  colormap_entry.push_back(mi::math::Color(0, 0.25098f, 1, 0));
  colormap_entry.push_back(mi::math::Color(0, 0.258824f, 1, 0));
  colormap_entry.push_back(mi::math::Color(0, 0.262745f, 1, 0));
  colormap_entry.push_back(mi::math::Color(0, 0.266667f, 1, 0.000351434f));
  colormap_entry.push_back(mi::math::Color(0, 0.270588f, 1, 0.00197421f));
  colormap_entry.push_back(mi::math::Color(0, 0.27451f, 1, 0.00359698f));
  colormap_entry.push_back(mi::math::Color(0, 0.278431f, 1, 0.00521975f));
  colormap_entry.push_back(mi::math::Color(0, 0.286275f, 1, 0.00684252f));
  colormap_entry.push_back(mi::math::Color(0, 0.290196f, 1, 0.0084653f));
  colormap_entry.push_back(mi::math::Color(0, 0.294118f, 1, 0.0100881f));
  colormap_entry.push_back(mi::math::Color(0, 0.298039f, 1, 0.0117108f));
  colormap_entry.push_back(mi::math::Color(0, 0.301961f, 1, 0.0133336f));
  colormap_entry.push_back(mi::math::Color(0, 0.305882f, 1, 0.0149564f));
  colormap_entry.push_back(mi::math::Color(0, 0.313726f, 1, 0.0165792f));
  colormap_entry.push_back(mi::math::Color(0, 0.317647f, 1, 0.0182019f));
  colormap_entry.push_back(mi::math::Color(0, 0.321569f, 1, 0.0198247f));
  colormap_entry.push_back(mi::math::Color(0, 0.32549f, 1, 0.0214475f));
  colormap_entry.push_back(mi::math::Color(0, 0.329412f, 1, 0.0230702f));
  colormap_entry.push_back(mi::math::Color(0, 0.333333f, 1, 0.024693f));
  colormap_entry.push_back(mi::math::Color(0, 0.341176f, 1, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(0, 0.345098f, 1, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(0, 0.34902f, 1, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(0, 0.352941f, 1, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(0, 0.356863f, 1, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(0, 0.360784f, 1, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(0, 0.368627f, 1, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(0, 0.372549f, 1, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(0, 0.376471f, 1, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(0, 0.380392f, 1, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(0, 0.384314f, 1, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(0, 0.392157f, 1, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(0, 0.396078f, 1, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(0, 0.4f, 1, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(0, 0.403922f, 1, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(0, 0.407843f, 1, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(0, 0.411765f, 1, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(0, 0.419608f, 1, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(0, 0.423529f, 1, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(0, 0.427451f, 1, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(0, 0.431373f, 1, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(0, 0.435294f, 1, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(0, 0.439216f, 1, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(0, 0.447059f, 1, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(0, 0.45098f, 1, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(0, 0.454902f, 1, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(0, 0.458824f, 1, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(0, 0.462745f, 1, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(0, 0.466667f, 1, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(0, 0.47451f, 1, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(0, 0.478431f, 1, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(0, 0.482353f, 1, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(0, 0.486275f, 1, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(0, 0.490196f, 1, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(0, 0.498039f, 1, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(0, 0.501961f, 1, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(0, 0.505882f, 1, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(0, 0.509804f, 1, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(0, 0.513726f, 1, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(0, 0.517647f, 1, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(0, 0.52549f, 1, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(0, 0.529412f, 1, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(0, 0.533333f, 1, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(0, 0.537255f, 1, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(0, 0.541176f, 1, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(0, 0.545098f, 1, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(0, 0.552941f, 1, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(0, 0.556863f, 1, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(0, 0.560784f, 1, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(0, 0.564706f, 1, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(0, 0.568627f, 1, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(0, 0.572549f, 1, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(0, 0.580392f, 1, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(0, 0.603922f, 1, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(0, 0.639216f, 1, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(0, 0.67451f, 1, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(0, 0.709804f, 1, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(0, 0.745098f, 1, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(0, 0.780392f, 1, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(0, 0.811765f, 1, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(0, 0.847059f, 1, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(0, 0.882353f, 1, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(0, 0.917647f, 1, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(0, 0.952941f, 1, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(0, 0.988235f, 1, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(0, 1, 0.976471f, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(0, 1, 0.941176f, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(0, 1, 0.905882f, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(0, 1, 0.87451f, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(0, 1, 0.839216f, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(0, 1, 0.803922f, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(0, 1, 0.768627f, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(0, 1, 0.733333f, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(0, 1, 0.698039f, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(0, 1, 0.662745f, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(0, 1, 0.627451f, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(0, 1, 0.592157f, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(0, 1, 0.560784f, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(0, 1, 0.52549f, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(0, 1, 0.490196f, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(0, 1, 0.454902f, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(0, 1, 0.419608f, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(0, 1, 0.384314f, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(0, 1, 0.34902f, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(0, 1, 0.313726f, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(0, 1, 0.278431f, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(0, 1, 0.231373f, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(0, 1, 0.164706f, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(0, 1, 0.0980392f, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(0, 1, 0.0313726f, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(0.0352941f, 1, 0, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(0.0980392f, 1, 0, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(0.164706f, 1, 0, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(0.231373f, 1, 0, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(0.298039f, 1, 0, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(0.364706f, 1, 0, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(0.431373f, 1, 0, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(0.498039f, 1, 0, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(0.564706f, 1, 0, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(0.631373f, 1, 0, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(0.698039f, 1, 0, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(0.764706f, 1, 0, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(0.831373f, 1, 0, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(0.898039f, 1, 0, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(0.964706f, 1, 0, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(1, 0.968627f, 0, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(1, 0.901961f, 0, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(1, 0.835294f, 0, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(1, 0.768627f, 0, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(1, 0.701961f, 0, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(1, 0.635294f, 0, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(1, 0.568627f, 0, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(1, 0.52549f, 0, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(1, 0.517647f, 0, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(1, 0.509804f, 0, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(1, 0.501961f, 0, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(1, 0.494118f, 0, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(1, 0.486275f, 0, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(1, 0.478431f, 0, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(1, 0.470588f, 0, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(1, 0.462745f, 0, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(1, 0.454902f, 0, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(1, 0.447059f, 0, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(1, 0.439216f, 0, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(1, 0.431373f, 0, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(1, 0.423529f, 0, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(1, 0.415686f, 0, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(1, 0.407843f, 0, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(1, 0.4f, 0, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(1, 0.392157f, 0, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(1, 0.384314f, 0, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(1, 0.376471f, 0, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(1, 0.368627f, 0, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(1, 0.360784f, 0, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(1, 0.352941f, 0, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(1, 0.345098f, 0, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(1, 0.337255f, 0, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(1, 0.329412f, 0, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(1, 0.321569f, 0, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(1, 0.313726f, 0, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(1, 0.305882f, 0, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(1, 0.298039f, 0, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(1, 0.290196f, 0, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(1, 0.282353f, 0, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(1, 0.27451f, 0, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(1, 0.266667f, 0, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(1, 0.258824f, 0, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(1, 0.25098f, 0, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(1, 0.243137f, 0, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(1, 0.235294f, 0, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(1, 0.227451f, 0, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(1, 0.219608f, 0, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(1, 0.211765f, 0, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(1, 0.203922f, 0, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(1, 0.196078f, 0, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(1, 0.188235f, 0, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(1, 0.180392f, 0, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(1, 0.172549f, 0, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(1, 0.164706f, 0, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(1, 0.156863f, 0, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(1, 0.14902f, 0, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(1, 0.141176f, 0, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(1, 0.133333f, 0, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(1, 0.12549f, 0, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(1, 0.117647f, 0, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(1, 0.109804f, 0, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(1, 0.101961f, 0, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(1, 0.0941176f, 0, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(1, 0.0862745f, 0, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(1, 0.0784314f, 0, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(1, 0.0705882f, 0, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(1, 0.0627451f, 0, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(1, 0.054902f, 0, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(1, 0.0470588f, 0, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(1, 0.0392157f, 0, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(1, 0.0313726f, 0, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(1, 0.0235294f, 0, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(1, 0.0156863f, 0, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(1, 0.00784314f, 0, 0.0263158f));
  colormap_entry.push_back(mi::math::Color(1, 0, 0, 0.0263158f));

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
