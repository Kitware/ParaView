/*=========================================================================

  Module:    vtkKWProcessStatistics.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkKWProcessStatistics.h"


#ifdef __linux
#include <sys/procfs.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/utsname.h> // int uname(struct utsname *buf);
#include <ctype.h> // int isdigit(int c);
#include <errno.h> // extern int errno;
#elif __hpux
#include <sys/param.h>
#include <sys/pstat.h>
#endif

#include "vtkObjectFactory.h"
#include "vtkWindows.h"

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
  this->TotalVirtualMemory = -1;
  this->TotalPhysicalMemory = -1;
  this->AvailableVirtualMemory = -1;
  this->AvailablePhysicalMemory = -1;
#ifdef __CYGWIN__
  return 0;
#elif _WIN32
  MEMORYSTATUS ms;
  GlobalMemoryStatus(&ms);

  unsigned long tv = ms.dwTotalVirtual;
  unsigned long tp = ms.dwTotalPhys;
  unsigned long av = ms.dwAvailVirtual;
  unsigned long ap = ms.dwAvailPhys;
  this->TotalVirtualMemory = tv>>10;
  this->TotalPhysicalMemory = tp>>10;
  this->AvailableVirtualMemory = av>>10;
  this->AvailablePhysicalMemory = ap>>10;
  return 1;
#elif __linux
  unsigned long tv=0;
  unsigned long tp=0;
  unsigned long av=0;
  unsigned long ap=0;
  
  char buffer[1024]; // for skipping unused lines
  
  int linuxMajor = 0;
  int linuxMinor = 0;
  
  // Find the Linux kernel version first
  struct utsname unameInfo;
  int errorFlag = uname(&unameInfo);
  if( errorFlag!=0 )
    {
    vtkErrorMacro("Problem calling uname(): " << strerror(errno) );
    return 0;
    }
 
  if( unameInfo.release!=0 && strlen(unameInfo.release)>=3 )
    {
    // release looks like "2.6.3-15mdk-i686-up-4GB"
    char majorChar=unameInfo.release[0];
    char minorChar=unameInfo.release[2];
    
    if( isdigit(majorChar) )
      {
      linuxMajor=majorChar-'0';
      }
    
    if( isdigit(minorChar) )
      {
      linuxMinor=minorChar-'0';
      }
    }
  
  FILE *fd = fopen("/proc/meminfo", "r" );
  if ( !fd ) 
    {
    vtkErrorMacro("Problem opening /proc/meminfo");
    return 0;
    }
  
  if( linuxMajor>=3 || ( (linuxMajor>=2) && (linuxMinor>=6) ) )
    {
    // new /proc/meminfo format since kernel 2.6.x
    // Rigorously, this test should check from the developping version 2.5.x
    // that introduced the new format...
    
    long freeMem;
    long buffersMem;
    long cachedMem;
    
    fscanf(fd,"MemTotal:%ld kB\n", &this->TotalPhysicalMemory);
    fscanf(fd,"MemFree:%ld kB\n", &freeMem);
    fscanf(fd,"Buffers:%ld kB\n", &buffersMem);
    fscanf(fd,"Cached:%ld kB\n", &cachedMem);
    
    this->AvailablePhysicalMemory=freeMem+cachedMem+buffersMem;
    
    // Skip SwapCached, Active, Inactive, HighTotal, HighFree, LowTotal
    // and LowFree.
    int i=0;
    while(i<7)
      {
      fgets(buffer, sizeof(buffer), fd); // skip a line
      ++i;
      }
    
    fscanf(fd,"SwapTotal:%ld kB\n", &this->TotalVirtualMemory);
    fscanf(fd,"SwapFree:%ld kB\n", &this->AvailableVirtualMemory);
    }
  else
    {
    // /proc/meminfo format for kernel older than 2.6.x
    
    unsigned long temp;
    unsigned long cachedMem;
    unsigned long buffersMem;
    fgets(buffer, sizeof(buffer), fd); // Skip "total: used:..."
    
    fscanf(fd, "Mem: %lu %lu %lu %lu %lu %lu\n",
         &tp, &temp, &ap, &temp, &buffersMem, &cachedMem);
    fscanf(fd, "Swap: %lu %lu %lu\n", &tv, &temp, &av);
    
    this->TotalVirtualMemory = tv>>10;
    this->TotalPhysicalMemory = tp>>10;
    this->AvailableVirtualMemory = av>>10;
    this->AvailablePhysicalMemory = (ap+buffersMem+cachedMem)>>10;
    }
  fclose( fd );
  return 1;
#elif __hpux
  unsigned long tv=0;
  unsigned long tp=0;
  unsigned long av=0;
  unsigned long ap=0;
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
