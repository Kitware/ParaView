include(CheckCSourceCompiles)
include(CheckCSourceRuns)
include(CheckIncludeFiles)
include(CheckTypeSize)
include(CheckSymbolExists)
include(CheckVariableExists)
include(CMake/MacroPushRequiredVars.cmake)
include(CMake/CheckStructHasMember.cmake)
include(CMake/CheckTypeExists.cmake)

if(WIN32)
  set(HAVE_DYNAMIC_LOADING 1)

else(WIN32)



macro(ADD_COND var cond item)
  if(${cond})
    set(${var} ${${var}} ${item})
  endif(${cond})
endmacro(ADD_COND)

check_include_files(asm/types.h HAVE_ASM_TYPES_H)
check_include_files(arpa/inet.h HAVE_ARPA_INET_H)
check_include_files(bluetooth/bluetooth.h HAVE_BLUETOOTH_BLUETOOTH_H)
check_include_files(bluetooth.h HAVE_BLUETOOTH_H)
check_include_files(conio.h HAVE_CONIO_H)
check_include_files(curses.h HAVE_CURSES_H)
check_include_files(direct.h HAVE_DIRECT_H)
check_include_files(dirent.h HAVE_DIRENT_H)
check_include_files(dlfcn.h HAVE_DLFCN_H)
check_include_files(errno.h HAVE_ERRNO_H)
check_include_files(fcntl.h HAVE_FCNTL_H)
check_include_files(grp.h HAVE_GRP_H)
check_include_files(inttypes.h HAVE_INTTYPES_H)
check_include_files(io.h HAVE_IO_H)
check_include_files(langinfo.h HAVE_LANGINFO_H)
check_include_files(libintl.h HAVE_LIBINTL_H)
check_include_files(libutil.h HAVE_LIBUTIL_H)
check_include_files(locale.h HAVE_LOCALE_H)

check_include_files(sys/socket.h HAVE_SYS_SOCKET_H)

set(LINUX_NETLINK_HEADERS)
add_cond(LINUX_NETLINK_HEADERS HAVE_ASM_TYPES_H  asm/types.h)
add_cond(LINUX_NETLINK_HEADERS HAVE_SYS_SOCKET_H sys/socket.h)
set(LINUX_NETLINK_HEADERS ${LINUX_NETLINK_HEADERS} linux/netlink.h)
check_include_files("${LINUX_NETLINK_HEADERS}" HAVE_LINUX_NETLINK_H)

check_include_files(memory.h HAVE_MEMORY_H)
check_include_files(ncurses.h HAVE_NCURSES_H)
check_include_files(ndir.h HAVE_NDIR_H)
check_include_files(netdb.h HAVE_NETDB_H)
check_include_files(netinet/in.h HAVE_NETINET_IN_H)
check_include_files(netpacket/packet.h HAVE_NETPACKET_PACKET_H)
check_include_files(poll.h HAVE_POLL_H)
check_include_files(process.h HAVE_PROCESS_H)
check_include_files(pthread.h HAVE_PTHREAD_H)
check_include_files(pty.h HAVE_PTY_H)
check_include_files(pwd.h HAVE_PWD_H)
check_include_files(readline/readline.h HAVE_READLINE_READLINE_H)
check_include_files(shadow.h HAVE_SHADOW_H)
check_include_files(signal.h HAVE_SIGNAL_H)
check_include_files(stdint.h HAVE_STDINT_H)
check_include_files(stdlib.h HAVE_STDLIB_H)
check_include_files(strings.h HAVE_STRINGS_H)
check_include_files(string.h HAVE_STRING_H)
check_include_files(stropts.h HAVE_STROPTS_H)
check_include_files(sysexits.h HAVE_SYSEXITS_H)
check_include_files(sys/audioio.h HAVE_SYS_AUDIOIO_H)
check_include_files(sys/bsdtty.h HAVE_SYS_BSDTTY_H)
check_include_files(sys/file.h HAVE_SYS_FILE_H)
check_include_files(sys/loadavg.h HAVE_SYS_LOADAVG_H)
check_include_files(sys/lock.h HAVE_SYS_LOCK_H)
check_include_files(sys/mkdev.h HAVE_SYS_MKDEV_H)
check_include_files(sys/mman.h HAVE_SYS_MMAN_H)
check_include_files(sys/modem.h HAVE_SYS_MODEM_H)
check_include_files(sys/ndir.h HAVE_SYS_NDIR_H)
check_include_files(sys/param.h HAVE_SYS_PARAM_H)
check_include_files(sys/poll.h HAVE_SYS_POLL_H)
check_include_files(sys/resource.h HAVE_SYS_RESOURCE_H)
check_include_files(sys/select.h HAVE_SYS_SELECT_H)
check_include_files(sys/statvfs.h HAVE_SYS_STATVFS_H)
check_include_files(sys/stat.h HAVE_SYS_STAT_H)
check_include_files(sys/timeb.h HAVE_SYS_TIMEB_H)
check_include_files(sys/times.h HAVE_SYS_TIMES_H)
check_include_files(sys/time.h HAVE_SYS_TIME_H)
check_include_files(sys/types.h HAVE_SYS_TYPES_H)
check_include_files(sys/un.h HAVE_SYS_UN_H)
check_include_files(sys/utsname.h HAVE_SYS_UTSNAME_H)
check_include_files(sys/wait.h HAVE_SYS_WAIT_H)
check_include_files(termios.h HAVE_TERMIOS_H)
check_include_files(term.h HAVE_TERM_H)
check_include_files(thread.h HAVE_THREAD_H)
check_include_files(unistd.h HAVE_UNISTD_H)
check_include_files(utime.h HAVE_UTIME_H)
check_include_files(wchar.h HAVE_WCHAR_H)
check_include_files("stdlib.h;stdarg.h;string.h;float.h" STDC_HEADERS)

check_include_files(stdarg.h HAVE_STDARG_PROTOTYPES)

find_file(HAVE_DEV_PTMX NAMES /dev/ptmx PATHS / NO_DEFAULT_PATH)
find_file(HAVE_DEV_PTC  NAMES /dev/ptc  PATHS / NO_DEFAULT_PATH)
message(STATUS "ptmx: ${HAVE_DEV_PTMX} ptc: ${HAVE_DEV_PTC}")

find_library(HAVE_LIBCURSES curses)
find_library(HAVE_LIBDL dl)
find_library(HAVE_LIBDLD dld)
find_library(HAVE_LIBIEEE ieee)
find_library(HAVE_LIBINTL intl)
find_library(HAVE_LIBM m)
find_library(HAVE_LIBNCURSES ncurses)
find_library(HAVE_LIBPTHREAD pthread)
find_library(HAVE_LIBREADLINE readline)
find_library(HAVE_LIBRESOLV resolv)
find_library(HAVE_LIBTERMCAP termcap)
find_library(HAVE_LIBUTIL    util)

set(CMAKE_REQUIRED_DEFINITIONS -D_FILE_OFFSET_BITS=64 -D_GNU_SOURCE=1 -D_BSD_TYPES=1 -DNETBSD_SOURCE=1 -D__BSD_VISIBLE=1)
set(CMAKE_EXTRA_INCLUDE_FILES stdio.h)

add_cond(CMAKE_REQUIRED_LIBRARIES HAVE_LIBM m)
add_cond(CMAKE_REQUIRED_LIBRARIES HAVE_LIBINTL intl)
add_cond(CMAKE_REQUIRED_LIBRARIES HAVE_LIBUTIL util)
add_cond(CMAKE_EXTRA_INCLUDE_FILES HAVE_WCHAR_H wchar.h)

check_type_size(double SIZEOF_DOUBLE)
check_type_size(float SIZEOF_FLOAT)
check_type_size(fpos_t SIZEOF_FPOS_T)
check_type_size(int SIZEOF_INT)
check_type_size(long SIZEOF_LONG)
check_type_size("long long" SIZEOF_LONG_LONG)
set(HAVE_LONG_LONG ${SIZEOF_LONG_LONG})
check_type_size(off_t SIZEOF_OFF_T)
check_type_size(pthread_t SIZEOF_PTHREAD_T)
check_type_size(short SIZEOF_SHORT)
check_type_size(size_t SIZEOF_SIZE_T)
check_type_size(ssize_t HAVE_SSIZE_T)
check_type_size(time_t SIZEOF_TIME_T)
check_type_size(uintptr_t SIZEOF_UINTPTR_T)
set(HAVE_UINTPTR_T ${SIZEOF_UINTPTR_T})
check_type_size("void *" SIZEOF_VOID_P)
check_type_size(wchar_t SIZEOF_WCHAR_T)
check_type_size(_Bool SIZEOF__BOOL)
set(HAVE_C99_BOOL ${SIZEOF__BOOL})

set(_LARGEFILE_SOURCE 1)
set(_FILE_OFFSET_BITS 64)

set(_NETBSD_SOURCE 1)
set(__BSD_VISIBLE 1)
set(_BSD_SOURCE 1)
set(_BSD_TYPES 1)
set(_GNU_SOURCE 1)
set(_XOPEN_SOURCE 600)
set(_XOPEN_SOURCE_EXTENDED 1)
set(_POSIX_C_SOURCE 200112L)

set(AIX_GENUINE_CPLUSPLUS 0)

set(WITH_DYLD 0)
set(WITH_NEXT_FRAMEWORK 0)
if(APPLE)
  set(WITH_DYLD 1)
  set(WITH_NEXT_FRAMEWORK 1)
endif(APPLE)


set(CFG_HEADERS )

add_cond(CFG_HEADERS HAVE_SYS_TYPES_H sys/types.h)
add_cond(CFG_HEADERS HAVE_SYS_TIME_H sys/time.h)
add_cond(CFG_HEADERS HAVE_SYS_FILE_H sys/file.h)
add_cond(CFG_HEADERS HAVE_SYS_POLL_H sys/poll.h)
add_cond(CFG_HEADERS HAVE_SYS_STATVFS_H sys/statvfs.h)
add_cond(CFG_HEADERS HAVE_SYS_STAT_H sys/stat.h)
add_cond(CFG_HEADERS HAVE_SYS_LOCK_H sys/lock.h)
add_cond(CFG_HEADERS HAVE_SYS_TIMEB_H sys/timeb.h)
add_cond(CFG_HEADERS HAVE_SYS_TIMES_H sys/times.h)
add_cond(CFG_HEADERS HAVE_SYS_UTSNAME_H sys/utsname.h)
add_cond(CFG_HEADERS HAVE_SYS_MMAN_H sys/mman.h)
add_cond(CFG_HEADERS HAVE_SYS_SOCKET_H sys/socket.h)
add_cond(CFG_HEADERS HAVE_SYS_WAIT_H sys/wait.h)
add_cond(CFG_HEADERS HAVE_PWD_H pwd.h)
add_cond(CFG_HEADERS HAVE_GRP_H grp.h)
add_cond(CFG_HEADERS HAVE_SHADOW_H shadow.h)
add_cond(CFG_HEADERS HAVE_LOCALE_H locale.h)
add_cond(CFG_HEADERS HAVE_LIBINTL_H libintl.h)
add_cond(CFG_HEADERS HAVE_FCNTL_H fcntl.h)
add_cond(CFG_HEADERS HAVE_SIGNAL_H signal.h)
add_cond(CFG_HEADERS HAVE_STDLIB_H stdlib.h)
add_cond(CFG_HEADERS HAVE_STRING_H string.h)
add_cond(CFG_HEADERS HAVE_UNISTD_H unistd.h)
add_cond(CFG_HEADERS HAVE_UTIME_H utime.h)
add_cond(CFG_HEADERS HAVE_WCHAR_H wchar.h)

if(HAVE_PTY_H)
  set(CFG_HEADERS ${CFG_HEADERS} pty.h utmp.h)
endif(HAVE_PTY_H)

set(CFG_HEADERS ${CFG_HEADERS} time.h stdio.h math.h)

check_symbol_exists(alarm        "${CFG_HEADERS}" HAVE_ALARM)
check_symbol_exists(altzone      "${CFG_HEADERS}" HAVE_ALTZONE)
check_symbol_exists(bind_textdomain_codeset "${CFG_HEADERS}" HAVE_BIND_TEXTDOMAIN_CODESET)
check_symbol_exists(chflags      "${CFG_HEADERS}" HAVE_CHFLAGS)
check_symbol_exists(chown        "${CFG_HEADERS}" HAVE_CHOWN)
check_symbol_exists(chroot       "${CFG_HEADERS}" HAVE_CHROOT)
check_symbol_exists(clock        "${CFG_HEADERS}" HAVE_CLOCK)
check_symbol_exists(confstr      "${CFG_HEADERS}" HAVE_CONFSTR)
check_symbol_exists(ctermid      "${CFG_HEADERS}" HAVE_CTERMID)
check_symbol_exists(ctermid_r    "${CFG_HEADERS}" HAVE_CTERMID_R)
check_symbol_exists(dup2         "${CFG_HEADERS}" HAVE_DUP2)
check_symbol_exists(execv        "${CFG_HEADERS}" HAVE_EXECV)
check_symbol_exists(fchdir       "${CFG_HEADERS}" HAVE_FCHDIR)
check_symbol_exists(fdatasync    "${CFG_HEADERS}" HAVE_FDATASYNC)
check_symbol_exists(flock        "${CFG_HEADERS}" HAVE_FLOCK)
check_symbol_exists(fork         "${CFG_HEADERS}" HAVE_FORK)
check_symbol_exists(forkpty      "${CFG_HEADERS}" HAVE_FORKPTY)
check_symbol_exists(fpathconf    "${CFG_HEADERS}" HAVE_FPATHCONF)
check_symbol_exists(fseek64      "${CFG_HEADERS}" HAVE_FSEEK64)
check_symbol_exists(fseeko       "${CFG_HEADERS}" HAVE_FSEEKO)
check_symbol_exists(fstatvfs     "${CFG_HEADERS}" HAVE_FSTATVFS)
check_symbol_exists(fsync        "${CFG_HEADERS}" HAVE_FSYNC)
check_symbol_exists(ftell64      "${CFG_HEADERS}" HAVE_FTELL64)
check_symbol_exists(ftello       "${CFG_HEADERS}" HAVE_FTELLO)
check_symbol_exists(ftime        "${CFG_HEADERS}" HAVE_FTIME)
check_symbol_exists(ftruncate    "${CFG_HEADERS}" HAVE_FTRUNCATE)
check_symbol_exists(getcwd       "${CFG_HEADERS}" HAVE_GETCWD)
check_symbol_exists(getc_unlocked   "${CFG_HEADERS}" HAVE_GETC_UNLOCKED)
check_symbol_exists(getgroups       "${CFG_HEADERS}" HAVE_GETGROUPS)
check_symbol_exists(getloadavg   "${CFG_HEADERS}" HAVE_GETLOADAVG)
check_symbol_exists(getlogin     "${CFG_HEADERS}" HAVE_GETLOGIN)
check_symbol_exists(getpagesize  "${CFG_HEADERS}" HAVE_GETPAGESIZE)
check_symbol_exists(getpgid      "${CFG_HEADERS}" HAVE_GETPGID)
check_symbol_exists(getpgrp      "${CFG_HEADERS}" HAVE_GETPGRP)
check_symbol_exists(getpid       "${CFG_HEADERS}" HAVE_GETPID)
check_symbol_exists(getpriority  "${CFG_HEADERS}" HAVE_GETPRIORITY)
check_symbol_exists(getpwent     "${CFG_HEADERS}" HAVE_GETPWENT)
check_symbol_exists(getsid       "${CFG_HEADERS}" HAVE_GETSID)
check_symbol_exists(getspent     "${CFG_HEADERS}" HAVE_GETSPENT)
check_symbol_exists(getspnam     "${CFG_HEADERS}" HAVE_GETSPNAM)
check_symbol_exists(gettimeofday "${CFG_HEADERS}" HAVE_GETTIMEOFDAY)
check_symbol_exists(getwd        "${CFG_HEADERS}" HAVE_GETWD)
check_symbol_exists(hypot        "${CFG_HEADERS}" HAVE_HYPOT)
check_symbol_exists(kill         "${CFG_HEADERS}" HAVE_KILL)
check_symbol_exists(killpg       "${CFG_HEADERS}" HAVE_KILLPG)
check_symbol_exists(lchflags     "${CFG_HEADERS}" HAVE_LCHFLAGS)
check_symbol_exists(lchown       "${CFG_HEADERS}" HAVE_LCHOWN)
check_symbol_exists(link         "${CFG_HEADERS}" HAVE_LINK)
check_symbol_exists(lstat        "${CFG_HEADERS}" HAVE_LSTAT)
check_symbol_exists(makedev      "${CFG_HEADERS}" HAVE_MAKEDEV)
check_symbol_exists(memmove      "${CFG_HEADERS}" HAVE_MEMMOVE)
check_symbol_exists(mkfifo       "${CFG_HEADERS}" HAVE_MKFIFO)
check_symbol_exists(mknod        "${CFG_HEADERS}" HAVE_MKNOD)
check_symbol_exists(mktime       "${CFG_HEADERS}" HAVE_MKTIME)
check_symbol_exists(mremap       "${CFG_HEADERS}" HAVE_MREMAP)
check_symbol_exists(nice         "${CFG_HEADERS}" HAVE_NICE)
check_symbol_exists(openpty      "${CFG_HEADERS}" HAVE_OPENPTY)
check_symbol_exists(pathconf     "${CFG_HEADERS}" HAVE_PATHCONF)
check_symbol_exists(pause        "${CFG_HEADERS}" HAVE_PAUSE)
check_symbol_exists(plock        "${CFG_HEADERS}" HAVE_PLOCK)
check_symbol_exists(poll         "${CFG_HEADERS}" HAVE_POLL)
check_symbol_exists(putenv       "${CFG_HEADERS}" HAVE_PUTENV)
check_symbol_exists(readlink     "${CFG_HEADERS}" HAVE_READLINK)
check_symbol_exists(realpath     "${CFG_HEADERS}" HAVE_REALPATH)
check_symbol_exists(select       "${CFG_HEADERS}" HAVE_SELECT)
check_symbol_exists(setegid      "${CFG_HEADERS}" HAVE_SETEGID)
check_symbol_exists(seteuid      "${CFG_HEADERS}" HAVE_SETEUID)
check_symbol_exists(setgid       "${CFG_HEADERS}" HAVE_SETGID)
check_symbol_exists(setgroups    "${CFG_HEADERS}" HAVE_SETGROUPS)
check_symbol_exists(setlocale    "${CFG_HEADERS}" HAVE_SETLOCALE)
check_symbol_exists(setpgid      "${CFG_HEADERS}" HAVE_SETPGID)
check_symbol_exists(setpgrp      "${CFG_HEADERS}" HAVE_SETPGRP)
check_symbol_exists(setregid     "${CFG_HEADERS}" HAVE_SETREGID)
check_symbol_exists(setreuid     "${CFG_HEADERS}" HAVE_SETREUID)
check_symbol_exists(setsid       "${CFG_HEADERS}" HAVE_SETSID)
check_symbol_exists(setuid       "${CFG_HEADERS}" HAVE_SETUID)
check_symbol_exists(setvbuf      "${CFG_HEADERS}" HAVE_SETVBUF)
check_symbol_exists(sigaction    "${CFG_HEADERS}" HAVE_SIGACTION)
check_symbol_exists(siginterrupt "${CFG_HEADERS}" HAVE_SIGINTERRUPT)
check_symbol_exists(sigrelse     "${CFG_HEADERS}" HAVE_SIGRELSE)
check_symbol_exists(snprintf     "${CFG_HEADERS}" HAVE_SNPRINTF)
check_symbol_exists(socketpair   "${CFG_HEADERS}" HAVE_SOCKETPAIR)
check_symbol_exists(statvfs      "${CFG_HEADERS}" HAVE_STATVFS)
check_symbol_exists(strdup       "${CFG_HEADERS}" HAVE_STRDUP)
check_symbol_exists(strerror     "${CFG_HEADERS}" HAVE_STRERROR)
check_symbol_exists(strftime     "${CFG_HEADERS}" HAVE_STRFTIME)
check_symbol_exists(symlink      "${CFG_HEADERS}" HAVE_SYMLINK)
check_symbol_exists(sysconf      "${CFG_HEADERS}" HAVE_SYSCONF)
check_symbol_exists(tcgetpgrp    "${CFG_HEADERS}" HAVE_TCGETPGRP)
check_symbol_exists(tcsetpgrp    "${CFG_HEADERS}" HAVE_TCSETPGRP)
check_symbol_exists(tempnam      "${CFG_HEADERS}" HAVE_TEMPNAM)
check_symbol_exists(timegm       "${CFG_HEADERS}" HAVE_TIMEGM)
check_symbol_exists(times        "${CFG_HEADERS}" HAVE_TIMES)
check_symbol_exists(tmpfile      "${CFG_HEADERS}" HAVE_TMPFILE)
check_symbol_exists(tmpnam       "${CFG_HEADERS}" HAVE_TMPNAM)
check_symbol_exists(tmpnam_r     "${CFG_HEADERS}" HAVE_TMPNAM_R)
check_symbol_exists(truncate     "${CFG_HEADERS}" HAVE_TRUNCATE)
check_symbol_exists(uname        "${CFG_HEADERS}" HAVE_UNAME)
check_symbol_exists(unsetenv     "${CFG_HEADERS}" HAVE_UNSETENV)
check_symbol_exists(utimes       "${CFG_HEADERS}" HAVE_UTIMES)
check_symbol_exists(wait3        "${CFG_HEADERS}" HAVE_WAIT3)
check_symbol_exists(wait4        "${CFG_HEADERS}" HAVE_WAIT4)
check_symbol_exists(waitpid      "${CFG_HEADERS}" HAVE_WAITPID)
check_symbol_exists(wcscoll      "${CFG_HEADERS}" HAVE_WCSCOLL)
check_symbol_exists(_getpty      "${CFG_HEADERS}" HAVE__GETPTY)

check_type_exists(DIR sys/dir.h HAVE_SYS_DIR_H)

check_struct_has_member("struct stat" st_mtim.tv_nsec "${CFG_HEADERS}" HAVE_STAT_TV_NSEC)
check_struct_has_member("struct stat" st_mtimensec "${CFG_HEADERS}"    HAVE_STAT_TV_NSEC2)
check_struct_has_member("struct stat" st_birthtime "${CFG_HEADERS}"    HAVE_STRUCT_STAT_ST_BIRTHTIME)
check_struct_has_member("struct stat" st_blksize "${CFG_HEADERS}"    HAVE_STRUCT_STAT_ST_BLKSIZE)
check_struct_has_member("struct stat" st_blocks  "${CFG_HEADERS}"    HAVE_STRUCT_STAT_ST_BLOCKS)
set(HAVE_ST_BLOCKS ${HAVE_STRUCT_STAT_ST_BLOCKS})
check_struct_has_member("struct stat" st_flags   "${CFG_HEADERS}"    HAVE_STRUCT_STAT_ST_FLAGS)
check_struct_has_member("struct stat" st_gen     "${CFG_HEADERS}"    HAVE_STRUCT_STAT_ST_GEN)
check_struct_has_member("struct stat" st_rdev    "${CFG_HEADERS}"    HAVE_STRUCT_STAT_ST_RDEV)


#######################################################################
#
# time
#
#######################################################################
check_struct_has_member("struct tm"   tm_zone    "${CFG_HEADERS}"    HAVE_STRUCT_TM_TM_ZONE)
check_struct_has_member("struct tm"   tm_zone    "${CFG_HEADERS}"    HAVE_STRUCT_TM_TM_ZONE)
set(HAVE_TM_ZONE ${HAVE_STRUCT_TM_TM_ZONE})

if(NOT HAVE_STRUCT_TM_TM_ZONE)
  check_variable_exists(tzname HAVE_TZNAME)
else(NOT HAVE_STRUCT_TM_TM_ZONE)
  set(HAVE_TZNAME 0)
endif(NOT HAVE_STRUCT_TM_TM_ZONE)

check_type_exists("struct tm" sys/time.h TM_IN_SYS_TIME)

check_c_source_compiles("#include <sys/time.h>\n int main() {gettimeofday((struct timeval*)0,(struct timezone*)0);}" GETTIMEOFDAY_WITH_TZ)

if(GETTIMEOFDAY_WITH_TZ)
  set(GETTIMEOFDAY_NO_TZ 0)
else(GETTIMEOFDAY_WITH_TZ)
  set(GETTIMEOFDAY_NO_TZ 1)
endif(GETTIMEOFDAY_WITH_TZ)

#######################################################################
#
# unicode 
#
#######################################################################

#ucs2
set(PY_UNICODE_TYPE "unsigned short")
set(HAVE_USABLE_WCHAR_T 0)
set(Py_UNICODE_SIZE 2)

if   ("${Py_UNICODE_SIZE}" STREQUAL "${SIZEOF_WCHAR_T}")
  set(PY_UNICODE_TYPE wchar_t)
  set(HAVE_USABLE_WCHAR_T 1)
  message(STATUS "Using wchar_t for unicode")
else ("${Py_UNICODE_SIZE}" STREQUAL "${SIZEOF_WCHAR_T}")

  if   ("${Py_UNICODE_SIZE}" STREQUAL "${SIZEOF_SHORT}")
    set(PY_UNICODE_TYPE "unsigned short")
    set(HAVE_USABLE_WCHAR_T 0)
    message(STATUS "Using unsigned short for unicode")
  else ("${Py_UNICODE_SIZE}" STREQUAL "${SIZEOF_SHORT}")

    if   ("${Py_UNICODE_SIZE}" STREQUAL "${SIZEOF_LONG}")
      set(PY_UNICODE_TYPE "unsigned long")
      set(HAVE_USABLE_WCHAR_T 0)
      message(STATUS "Using unsigned long for unicode")
    else ("${Py_UNICODE_SIZE}" STREQUAL "${SIZEOF_LONG}")

      if(Py_USING_UNICODE)
        message(SEND_ERROR "No usable unicode type found, disable Py_USING_UNICODE to be able to build Python")
      else(Py_USING_UNICODE)
        message(STATUS "No usable unicode type found")
      endif(Py_USING_UNICODE)

    endif("${Py_UNICODE_SIZE}" STREQUAL "${SIZEOF_LONG}")

  endif("${Py_UNICODE_SIZE}" STREQUAL "${SIZEOF_SHORT}")

endif("${Py_UNICODE_SIZE}" STREQUAL "${SIZEOF_WCHAR_T}")




#######################################################################
#
# networking tests
#
#######################################################################
macro_push_required_vars()
set(CFG_HEADERS_SAVE ${CFG_HEADERS})
add_cond(CFG_HEADERS HAVE_NETDB_H netdb.h)
add_cond(CFG_HEADERS HAVE_NETDB_H netinet/in.h)
add_cond(CFG_HEADERS HAVE_ARPA_INET_H arpa/inet.h)

check_symbol_exists(gai_strerror    "${CFG_HEADERS}" HAVE_GAI_STRERROR)
check_symbol_exists(getaddrinfo     "${CFG_HEADERS}" HAVE_GETADDRINFO)
check_symbol_exists(gethostbyname   "${CFG_HEADERS}" HAVE_GETHOSTBYNAME)
#check_symbol_exists(gethostbyname_r "${CFG_HEADERS}" HAVE_GETHOSTBYNAME_R) # see end of file
check_symbol_exists(getnameinfo     "${CFG_HEADERS}" HAVE_GETNAMEINFO)
check_symbol_exists(getpeername     "${CFG_HEADERS}" HAVE_GETPEERNAME)
check_symbol_exists(hstrerror       "${CFG_HEADERS}" HAVE_HSTRERROR)
check_symbol_exists(inet_aton       "${CFG_HEADERS}" HAVE_INET_ATON)
check_symbol_exists(inet_pton       "${CFG_HEADERS}" HAVE_INET_PTON)

check_type_exists("struct addrinfo" "${CFG_HEADERS}" HAVE_ADDRINFO)
check_struct_has_member(sockaddr sa_len "${CFG_HEADERS}" HAVE_SOCKADDR_SA_LEN )
check_type_exists("struct sockaddr_storage" "${CFG_HEADERS}" HAVE_SOCKADDR_STORAGE)

set(CFG_HEADERS ${CFG_HEADERS_SAVE})
macro_pop_required_vars()


#######################################################################
#
# multithreading stuff
#
#######################################################################
macro_push_required_vars()
set(CFG_HEADERS_SAVE ${CFG_HEADERS})

set(ATHEOS_THREADS 0)
set(BEOS_THREADS 0)
set(C_THREADS 0)
set(HURD_C_THREADS 0)
set(MACH_C_THREADS 0)
set(HAVE_PTH 0) # GNU PTH threads

set(HAVE_PTHREAD_DESTRUCTOR 0) # for Solaris 2.6
add_cond(CFG_HEADERS  HAVE_PTHREAD_H  pthread.h)
add_cond(CMAKE_REQUIRED_LIBRARIES  HAVE_LIBPTHREAD  ${HAVE_LIBPTHREAD})

check_symbol_exists(pthread_init "${CFG_HEADERS}" HAVE_PTHREAD_INIT)
check_symbol_exists(pthread_sigmask "${CFG_HEADERS}" HAVE_PTHREAD_SIGMASK)

set(CFG_HEADERS ${CFG_HEADERS_SAVE})
macro_pop_required_vars()

if(CMAKE_SYSTEM MATCHES BlueGene)
  set(WITH_THREAD OFF CACHE STRING "System doesn't support multithreading" FORCE)
endif(CMAKE_SYSTEM MATCHES BlueGene)


#######################################################################
#
# readline tests
#
#######################################################################
if(HAVE_READLINE_READLINE_H)
  macro_push_required_vars()
  set(CFG_HEADERS_SAVE ${CFG_HEADERS})

  add_cond(CFG_HEADERS HAVE_READLINE_READLINE_H readline/readline.h)
  add_cond(CMAKE_REQUIRED_LIBRARIES HAVE_LIBREADLINE ${HAVE_LIBREADLINE})
  check_symbol_exists(rl_callback_handler_install "${CFG_HEADERS}" HAVE_RL_CALLBACK)
  check_symbol_exists(rl_catch_signals            "${CFG_HEADERS}" HAVE_RL_CATCH_SIGNAL)
  check_symbol_exists(rl_completion_append_character "${CFG_HEADERS}" HAVE_RL_COMPLETION_APPEND_CHARACTER)
  check_symbol_exists(rl_completion_matches       "${CFG_HEADERS}" HAVE_RL_COMPLETION_MATCHES)
  check_symbol_exists(rl_pre_input_hook           "${CFG_HEADERS}" HAVE_RL_PRE_INPUT_HOOK)

  set(CFG_HEADERS ${CFG_HEADERS_SAVE})
  macro_pop_required_vars()
endif(HAVE_READLINE_READLINE_H)


#######################################################################
#
# curses tests
#
#######################################################################
if(HAVE_CURSES_H)
  macro_push_required_vars()
  set(CFG_HEADERS_SAVE ${CFG_HEADERS})

  set(CFG_HEADERS ${CFG_HEADERS} curses.h)
  add_cond(CMAKE_REQUIRED_LIBRARIES HAVE_LIBCURSES ${HAVE_LIBCURSES})
  check_symbol_exists(is_term_resized "${CFG_HEADERS}" HAVE_CURSES_IS_TERM_RESIZED)
  check_symbol_exists(resizeterm      "${CFG_HEADERS}" HAVE_CURSES_RESIZETERM)
  check_symbol_exists(resize_term     "${CFG_HEADERS}" HAVE_CURSES_RESIZE_TERM)
  check_struct_has_member(WINDOW _flags   "${CFG_HEADERS}" WINDOW_HAS_FLAGS)

  check_c_source_compiles("#include <curses.h>\n int main() {int i; i = mvwdelch(0,0,0);}" MVWDELCH_IS_EXPRESSION)

  set(CFG_HEADERS ${CFG_HEADERS_SAVE})
  macro_pop_required_vars()
endif(HAVE_CURSES_H)


#######################################################################
#
# dynamic loading
#
#######################################################################
if(HAVE_DLFCN_H)
  macro_push_required_vars()
  set(CFG_HEADERS_SAVE ${CFG_HEADERS})

  set(CFG_HEADERS ${CFG_HEADERS} dlfcn.h)
  add_cond(CMAKE_REQUIRED_LIBRARIES HAVE_LIBDL ${HAVE_LIBDL})
  check_symbol_exists(dlopen          "${CFG_HEADERS}" HAVE_DLOPEN)

  set(CFG_HEADERS ${CFG_HEADERS_SAVE})
  macro_pop_required_vars()
endif(HAVE_DLFCN_H)


if(HAVE_DLOPEN) # OR .... )
  set(HAVE_DYNAMIC_LOADING 1)
else(HAVE_DLOPEN)
  set(HAVE_DYNAMIC_LOADING 0)
endif(HAVE_DLOPEN)


#######################################################################
#
# check some types
#
#######################################################################
check_type_exists(gid_t sys/types.h gid_t)
if(NOT gid_t)
  set(gid_t int)
else(NOT gid_t)
  set(gid_t 0)
endif(NOT gid_t)

check_type_exists(mode_t sys/types.h mode_t)
if(NOT mode_t)
  set(mode_t int)
else(NOT mode_t)
  set(mode_t 0)
endif(NOT mode_t)

check_type_exists(off_t sys/types.h off_t)
if(NOT off_t)
  set(off_t "long int")
else(NOT off_t)
  set(off_t 0)
endif(NOT off_t)

check_type_exists(pid_t sys/types.h pid_t)
if(NOT pid_t)
  set(pid_t int)
else(NOT pid_t)
  set(pid_t 0)
endif(NOT pid_t)

check_type_exists(size_t sys/types.h size_t)
if(NOT size_t)
  set(size_t "unsigned int")
else(NOT size_t)
  set(size_t 0)
endif(NOT size_t)

check_type_exists(socklen_t sys/socket.h socklen_t)
if(NOT socklen_t)
  set(socklen int)
else(NOT socklen_t)
  set(socklen_t 0)
endif(NOT socklen_t)

check_type_exists(uid_t sys/types.h uid_t)
if(NOT uid_t)
  set(uid_t int)
else(NOT uid_t)
  set(uid_t 0)
endif(NOT uid_t)

check_type_exists(clock_t time.h clock_t)
if(NOT clock_t)
  set(clock_t long)
else(NOT clock_t)
  set(clock_t 0)
endif(NOT clock_t)


check_c_source_compiles("int main() {const int i;}" const_WORKS)
if(NOT const_WORKS)
  set(const 1)
else(NOT const_WORKS)
  set(const 0)
endif(NOT const_WORKS)

check_c_source_compiles("int main() {signed int i;}" signed_WORKS)
if(NOT signed_WORKS)
  set(signed 1)
else(NOT signed_WORKS)
  set(signed 0)
endif(NOT signed_WORKS)

check_c_source_compiles("int main() {volatile int i;}" volatile_WORKS)
if(NOT volatile_WORKS)
  set(volatile 1)
else(NOT volatile_WORKS)
  set(volatile 0)
endif(NOT volatile_WORKS)

if(HAVE_STDARG_PROTOTYPES)
   set(vaargsHeader "stdarg.h")
else(HAVE_STDARG_PROTOTYPES)
   set(vaargsHeader "varargs.h")
endif(HAVE_STDARG_PROTOTYPES)
check_c_source_compiles("#include <${vaargsHeader}>\n int main() {va_list list1, list2; list1 = list2;}" NOT_VA_LIST_IS_ARRAY)
if(NOT_VA_LIST_IS_ARRAY)
  set(VA_LIST_IS_ARRAY 0)
else(NOT_VA_LIST_IS_ARRAY)
  set(VA_LIST_IS_ARRAY 1)
endif(NOT_VA_LIST_IS_ARRAY)


#######################################################################
#
# tests for bugs and other stuff
#
#######################################################################

check_c_source_compiles("
        void f(char*,...)__attribute((format(PyArg_ParseTuple, 1, 2))) {}; 
        int main() {f(NULL);} "
        HAVE_ATTRIBUTE_FORMAT_PARSETUPLE)

set(CMAKE_REQUIRED_INCLUDES ${CFG_HEADERS})
check_c_source_compiles("#include <unistd.h>\n int main() {getpgrp(0);}" GETPGRP_HAVE_ARG)

check_c_source_runs("int main() {
        int val1 = nice(1); 
        if (val1 != -1 && val1 == nice(2)) exit(0);
        exit(1);}" HAVE_BROKEN_NICE)

check_c_source_runs(" #include <poll.h>
    int main () {
    struct pollfd poll_struct = { 42, POLLIN|POLLPRI|POLLOUT, 0 }; close (42);
    int poll_test = poll (&poll_struct, 1, 0);
    if (poll_test < 0) { exit(0); }
    else if (poll_test == 0 && poll_struct.revents != POLLNVAL) { exit(0); }
    else { exit(1); } }" 
    HAVE_BROKEN_POLL)

if(HAVE_SYS_TIME_H)
  check_include_files("sys/time.h;time.h" TIME_WITH_SYS_TIME)
else(HAVE_SYS_TIME_H)
  set(TIME_WITH_SYS_TIME 0)
endif(HAVE_SYS_TIME_H)

if(HAVE_SYS_TIME_H AND HAVE_SYS_SELECT_H)
  check_include_files("sys/select.h;sys/time.h" SYS_SELECT_WITH_SYS_TIME)
else(HAVE_SYS_TIME_H AND HAVE_SYS_SELECT_H)
  set(SYS_SELECT_WITH_SYS_TIME 0)
endif(HAVE_SYS_TIME_H AND HAVE_SYS_SELECT_H)


##########################################################

find_package(ZLIB)
if(ZLIB_FOUND)
  macro_push_required_vars()
  set(CFG_HEADERS_SAVE ${CFG_HEADERS})

  set(CFG_HEADERS ${CFG_HEADERS} zlib.h)
  add_cond(CMAKE_REQUIRED_LIBRARIES ZLIB_FOUND ${ZLIB_LIBRARIES})
  check_symbol_exists(inflateCopy      "${CFG_HEADERS}" HAVE_ZLIB_COPY)

  set(CFG_HEADERS ${CFG_HEADERS_SAVE})
  macro_pop_required_vars()
endif(ZLIB_FOUND)

############################################

# setup the python platform
set(PY_PLATFORM generic)

if(CMAKE_SYSTEM MATCHES Linux)
  set(PY_PLATFORM linux2)
endif(CMAKE_SYSTEM MATCHES Linux)

if(CMAKE_SYSTEM MATCHES Darwin)
  set(PY_PLATFORM darwin)
endif(CMAKE_SYSTEM MATCHES Darwin)

if(CMAKE_SYSTEM MATCHES FreeBSD)
  set(PY_PLATFORM freebsd5)  # which version to use ?
endif(CMAKE_SYSTEM MATCHES FreeBSD)

if(CMAKE_SYSTEM MATCHES NetBSD)
  set(PY_PLATFORM netbsd1)
endif(CMAKE_SYSTEM MATCHES NetBSD)

if(CMAKE_SYSTEM MATCHES AIX)
  set(PY_PLATFORM aix4)
endif(CMAKE_SYSTEM MATCHES AIX)

if(CMAKE_SYSTEM MATCHES BeOS)
  set(PY_PLATFORM beos5)
endif(CMAKE_SYSTEM MATCHES BeOS)

if(CMAKE_SYSTEM MATCHES IRIX)
  set(PY_PLATFORM irix6)
endif(CMAKE_SYSTEM MATCHES IRIX)

if(CMAKE_SYSTEM MATCHES SunOS)
  set(PY_PLATFORM sunos5)
endif(CMAKE_SYSTEM MATCHES SunOS)

if(CMAKE_SYSTEM MATCHES UnixWare)
  set(PY_PLATFORM unixware7)
endif(CMAKE_SYSTEM MATCHES UnixWare)

if(CMAKE_SYSTEM MATCHES Windows)
  set(PY_PLATFORM win32)
endif(CMAKE_SYSTEM MATCHES Windows)

# todo 
set(HAVE_UCS4_TCL 0)
set(HAVE_PROTOTYPES 1)
set(PTHREAD_SYSTEM_SCHED_SUPPORTED 1)
set(RETSIGTYPE void)
set(HAVE_WORKING_TZSET 1)
set(HAVE_DECL_TZNAME 0) # no test in python sources
set(HAVE_DEVICE_MACROS ${HAVE_MAKEDEV})

set(HAVE_GETHOSTBYNAME_R 0)
set(HAVE_GETHOSTBYNAME_R_3_ARG 0)
set(HAVE_GETHOSTBYNAME_R_5_ARG 0)
set(HAVE_GETHOSTBYNAME_R_6_ARG 0)

endif(WIN32)
