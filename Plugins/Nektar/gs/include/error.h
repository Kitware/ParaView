/**********************************error.h*************************************
SPARSE GATHER-SCATTER PACKAGE: bss_malloc bss_malloc ivec error comm gs queue

Author: Henry M. Tufo III

e-mail: hmt@cs.brown.edu

snail-mail:
Division of Applied Mathematics
Brown University
Providence, RI 02912

Last Modification:
6.21.97
**********************************error.h*************************************/

/**********************************error.h*************************************
File Description:
-----------------

**********************************error.h*************************************/
#ifndef _error_h
#define _error_h



/**********************************error.h*************************************
Function: error_msg_fatal()

Input : formatted string and arguments.
Output: conversion printed to stdout.
Return: na.
Description: prints error message and terminates program.
Usage: error_msg_fatal("this is my %d'st test",test_num)
**********************************error.h*************************************/
extern void error_msg_fatal(char *msg, ...);



/**********************************error.h*************************************
Function: error_msg_warning()

Input : formatted string and arguments.
Output: conversion printed to stdout.
Return: na.
Description: prints error message.
Usage: error_msg_warning("this is my %d'st test",test_num)
**********************************error.h*************************************/
extern void error_msg_warning(char *msg, ...);

#endif
