/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2008 SciberQuest Inc.
*/
#include "OSXSystemInterface.h"

// #include <sys/types.h>
// #include <sys/sysctl.h>
// #include <sys/vmmeter.h>
// #include <mach/mach_init.h>
// #include <mach/mach_host.h>
// #include <mach/mach_port.h>
// #include <mach/mach_traps.h>
// #include <mach/task_info.h>
// #include <mach/thread_info.h>
// #include <mach/thread_act.h>
// #include <mach/vm_region.h>
// #include <mach/vm_map.h>
// #include <mach/task.h>
// #include <mach/shared_memory_server.h>
//
// typedef struct vmtotal vmtotal_t;
//
// typedef struct {
//      /* dynamic process information */
//     size_t rss, vsize;
//     double utime, stime;
// } RunProcDyn;
//
// /* On Mac OS X, the only way to get enough information is to become root. Pretty frustrating!*/
// int run_get_dynamic_proc_info(pid_t pid, RunProcDyn *rpd)
// {
//     task_t task;
//     kern_return_t error;
//     mach_msg_type_number_t count;
//     thread_array_t thread_table;
//     thread_basic_info_t thi;
//     thread_basic_info_data_t thi_data;
//     unsigned table_size;
//     struct task_basic_info ti;
// 
//     error = task_for_pid(mach_task_self(), pid, &task);
//     if (error != KERN_SUCCESS) {
//         /* fprintf(stderr, "++ Probably you have to set suid or become root.\n"); */
//         rpd->rss = rpd->vsize = 0;
//         rpd->utime = rpd->stime = 0;
//         return 0;
//     }
//     count = TASK_BASIC_INFO_COUNT;
//     error = task_info(task, TASK_BASIC_INFO, (task_info_t)&ti, &count);
//     assert(error == KERN_SUCCESS);
//     { /* adapted from ps/tasks.c */
//         vm_region_basic_info_data_64_t b_info;
//         vm_address_t address = GLOBAL_SHARED_TEXT_SEGMENT;
//         vm_size_t size;
//         mach_port_t object_name;
//         count = VM_REGION_BASIC_INFO_COUNT_64;
//         error = vm_region_64(task, &address, &size, VM_REGION_BASIC_INFO,
//                                                  (vm_region_info_t)&b_info, &count, &object_name);
//         if (error == KERN_SUCCESS) {
//                 if (b_info.reserved && size == (SHARED_TEXT_REGION_SIZE) &&
//                 ti.virtual_size > (SHARED_TEXT_REGION_SIZE + SHARED_DATA_REGION_SIZE))
//                 {
//                         ti.virtual_size -= (SHARED_TEXT_REGION_SIZE + SHARED_DATA_REGION_SIZE);
//                 }
//         }
//         rpd->rss = ti.resident_size;
//         rpd->vsize = ti.virtual_size;
//     }
// 
//     mach_port_deallocate(mach_task_self(), task);
//     return 0;
// }
