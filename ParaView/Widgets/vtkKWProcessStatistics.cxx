/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWProcessStatistics.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkKWProcessStatistics.h"

vtkCxxRevisionMacro(vtkKWProcessStatistics, "1.2");

#ifndef _WIN32
#include <sys/procfs.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
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
  this->TotalVirtualMemory = -1;
  this->TotalPhysicalMemory = -1;
  this->AvailableVirtualMemory = -1;
  this->AvailablePhysicalMemory = -1;
#ifdef __CYGWIN__
  return 0;
#elif _WIN32  
  MEMORYSTATUS ms;
  GlobalMemoryStatus(&ms);
  this->TotalVirtualMemory = ms.dwTotalVirtual;
  this->TotalPhysicalMemory = ms.dwTotalPhys;
  this->AvailableVirtualMemory = ms.dwAvailVirtual;
  this->AvailablePhysicalMemory = ms.dwAvailPhys;
  return 1;
#elif __linux
  FILE *fd;
  fd = fopen("/proc/meminfo", "r" );
  if ( !fd ) 
    {
    cout << "Problem opening /proc/meminfo" << endl;
    return 0;
    }
  long temp;
  char buffer[1024];
  fgets(buffer, sizeof(buffer), fd);
  fscanf(fd, "Mem: %ld %ld %ld %ld %ld %ld\n",
	 &this->TotalPhysicalMemory, &temp, &this->AvailablePhysicalMemory,
	 &temp, &temp, &temp);
  fscanf(fd, "Swap: %ld %ld %ld\n",
	 &this->TotalVirtualMemory, &temp, &this->AvailableVirtualMemory);  
  fclose( fd );
  return 1;
#elif __hpux
  struct pst_static pst;
  struct pst_dynamic pdy;
     
  unsigned long ps = 0;
  if (pstat_getstatic(&pst, sizeof(pst), (size_t) 1, 0) != -1)
    {
    ps = pst.page_size;
    this->TotalPhysicalMemory =  pst.physical_memory *ps;
    this->TotalVirtualMemory = (pst.physical_memory + pst.pst_maxmem) * ps;
    if (pstat_getdynamic(&pdy, sizeof(pdy), (size_t) 1, 0) != -1)
      {
      this->AvailablePhysicalMemory = this->TotalPhysicalMemory -
	pdy.psd_rm * ps;
      this->AvailableVirtualMemory = this->TotalVirtualMemory -
	pdy.psd_vm;
      return 1;
      }
    }
  return 0;
#else
  return 0;
#endif
}

