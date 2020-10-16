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
// After finishing using this shared memory pointer unmap_shm() must be called.
inline mi::Uint8* get_vol_shm(const std::string& shmname, const mi::Uint64& shared_seg_size)
{
#ifdef _WIN32
  HANDLE hMapFile;
  ULARGE_INTEGER liMaximumSize;
  liMaximumSize.QuadPart = shared_seg_size;
  hMapFile = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, liMaximumSize.HighPart,
    liMaximumSize.LowPart, shmname.c_str());

  if (hMapFile == NULL)
  {
    ERROR_LOG << "shm_open(): Could not create file mapping object, shared memory: " << shmname
              << ".";
    return NULL;
  }

  void* shm_volume = MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, shared_seg_size);

  if (shm_volume == NULL)
  {
    CloseHandle(hMapFile);

    ERROR_LOG << "shm_open(): Could not map view of file, shared memory: " << shmname << ".";
    return NULL;
  }

  CloseHandle(hMapFile);

  return reinterpret_cast<mi::Uint8*>(shm_volume);
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

  void* shm_volume = mmap(NULL, shared_seg_size, PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0);
  if (shm_volume == NULL)
  {
    ERROR_LOG << "The function mmap() failed in vtknvindex_representation, shared memory: "
              << shmname << ".";
    return NULL;
  }
  close(shmfd);

  return reinterpret_cast<mi::Uint8*>(shm_volume);
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
