/*=========================================================================

Copyright (c) 1998-2003 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither the name of Kitware nor the names of any contributors may be used
   to endorse or promote products derived from this software without specific
   prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkKWProcessStatistics.h"

vtkCxxRevisionMacro(vtkKWProcessStatistics, "1.4");

#ifdef __linux
#include <sys/procfs.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#elif __hpux
#include <sys/param.h>
#include <sys/pstat.h>
#endif

#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkKWProcessStatistics);

#ifndef _WIN32
/* This mess was copied from the GNU getpagesize.h.  */
#ifndef HAVE_GETPAGESIZE
# ifdef HAVE_UNISTD_H
#  include <unistd.h>
# endif

/* Assume that all systems that can run configure have sys/param.h.  */
# ifndef HAVE_SYS_PARAM_H
#  define HAVE_SYS_PARAM_H 1
# endif

# ifdef _SC_PAGESIZE
#  define getpagesize() sysconf(_SC_PAGESIZE)
# else /* no _SC_PAGESIZE */
#  ifdef HAVE_SYS_PARAM_H
#   include <sys/param.h>
#   ifdef EXEC_PAGESIZE
#    define getpagesize() EXEC_PAGESIZE
#   else /* no EXEC_PAGESIZE */
#    ifdef NBPG
#     define getpagesize() NBPG * CLSIZE
#     ifndef CLSIZE
#      define CLSIZE 1
#     endif /* no CLSIZE */
#    else /* no NBPG */
#     ifdef NBPC
#      define getpagesize() NBPC
#     else /* no NBPC */
#      ifdef PAGESIZE
#       define getpagesize() PAGESIZE
#      endif /* PAGESIZE */
#     endif /* no NBPC */
#    endif /* no NBPG */
#   endif /* no EXEC_PAGESIZE */
#  else /* no HAVE_SYS_PARAM_H */
#   define getpagesize() 8192   /* punt totally */
#  endif /* no HAVE_SYS_PARAM_H */
# endif /* no _SC_PAGESIZE */

#endif /* no HAVE_GETPAGESIZE */
#endif // _WIN32

// Construct the ProcessStatistics with eight points.
vtkKWProcessStatistics::vtkKWProcessStatistics()
{
}


int vtkKWProcessStatistics::GetProcessSizeInBytes()
{

#ifdef _solaris
  prpsinfo  psinfo;
  int       fd;
  char      pname[1024];
  int       pagesize;
  pid_t     pid;

  // Get out process id
  pid = getpid();

  // Get the size of a page in bytes
  pagesize = getpagesize();

  // Open the /proc/<pid> file and query the
  // process info
  sprintf( pname, "/proc/%d", pid );
  fd = open( pname, O_RDONLY );
  if (fd != -1)
    {
    psinfo.pr_size = 0;
    ioctl( fd, PIOCPSINFO, &psinfo );
    close( fd );
    }
  else
    {
      vtkErrorMacro(<< "Cannot get size of " << pname);
      return 0;
    }

  // The size in bytes is the page size of the process times
  // the size of a page in bytes
  return psinfo.pr_size * pagesize;
#else
  return 0;
#endif

}

float vtkKWProcessStatistics::GetProcessCPUTimeInMilliseconds()
{

#ifdef _solaris
  prpsinfo  psinfo;
  int       fd;
  char      pname[1024];
  pid_t     pid;

  // Get out process id
  pid = getpid();

  // Open the /proc/<pid> file and query the
  // process info
  sprintf( pname, "/proc/%d", pid );
  fd = open( pname, O_RDONLY );
  ioctl( fd, PIOCPSINFO, &psinfo );
  close( fd );

  return 
    (float) psinfo.pr_time.tv_sec * 1000.0 + 
    (float) psinfo.pr_time.tv_nsec / 1000000.0;
#else
  return 0.0;
#endif

}

int vtkKWProcessStatistics::QueryMemory()
{
  unsigned long tv=0;
  unsigned long tp=0;
  unsigned long av=0;
  unsigned long ap=0;

  this->TotalVirtualMemory = -1;
  this->TotalPhysicalMemory = -1;
  this->AvailableVirtualMemory = -1;
  this->AvailablePhysicalMemory = -1;
#ifdef __CYGWIN__
  return 0;
#elif _WIN32  
  MEMORYSTATUS ms;
  GlobalMemoryStatus(&ms);
  
  tv = ms.dwTotalVirtual;
  tp = ms.dwTotalPhys;
  av = ms.dwAvailVirtual;
  ap = ms.dwAvailPhys;
  this->TotalVirtualMemory = tv>>10;
  this->TotalPhysicalMemory = tp>>10;
  this->AvailableVirtualMemory = av>>10;
  this->AvailablePhysicalMemory = ap>>10;
  return 1;
#elif __linux
  FILE *fd;
  fd = fopen("/proc/meminfo", "r" );
  if ( !fd ) 
    {
    vtkErrorMacro("Problem opening /proc/meminfo");
    return 0;
    }
  unsigned long temp;
  char buffer[1024];
  fgets(buffer, sizeof(buffer), fd);
  fscanf(fd, "Mem: %lu %lu %lu %lu %lu %lu\n",
         &tp, &temp, &ap, &temp, &temp, &temp);
  fscanf(fd, "Swap: %lu %lu %lu\n", &tv, &temp, &av);  
  fclose( fd );
  this->TotalVirtualMemory = tv>>10;
  this->TotalPhysicalMemory = tp>>10;
  this->AvailableVirtualMemory = av>>10;
  this->AvailablePhysicalMemory = ap>>10;
  return 1;
#elif __hpux
  struct pst_static pst;
  struct pst_dynamic pdy;
     
  unsigned long ps = 0;
  if (pstat_getstatic(&pst, sizeof(pst), (size_t) 1, 0) != -1)
    {
    ps = pst.page_size;
    tp =  pst.physical_memory *ps;
    tv = (pst.physical_memory + pst.pst_maxmem) * ps;
    if (pstat_getdynamic(&pdy, sizeof(pdy), (size_t) 1, 0) != -1)
      {
      ap = tp - pdy.psd_rm * ps;
      av = tv - pdy.psd_vm;
      this->TotalVirtualMemory = tv>>10;
      this->TotalPhysicalMemory = tp>>10;
      this->AvailableVirtualMemory = av>>10;
      this->AvailablePhysicalMemory = ap>>10;
      return 1;
      }
    }
  return 0;
#else
  return 0;
#endif
}

long vtkKWProcessStatistics::GetTotalVirtualMemory() 
{ 
  this->QueryMemory(); 
  return this->TotalVirtualMemory; 
}

long vtkKWProcessStatistics::GetAvailableVirtualMemory() 
{ 
  this->QueryMemory(); 
  return this->AvailableVirtualMemory; 
}

long vtkKWProcessStatistics::GetTotalPhysicalMemory() 
{ 
  this->QueryMemory(); 
  return this->TotalPhysicalMemory; 
}

long vtkKWProcessStatistics::GetAvailablePhysicalMemory() 
{ 
  this->QueryMemory(); 
  return this->AvailablePhysicalMemory; 
}  

//----------------------------------------------------------------------------
void vtkKWProcessStatistics::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}



