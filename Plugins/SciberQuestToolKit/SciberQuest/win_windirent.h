/*
    Copyright Kevlin Henney, 1997, 2003. All rights reserved.

    Permission to use, copy, modify, and distribute this software and its
    documentation for any purpose is hereby granted without fee, provided
    that this copyright and permissions notice appear in all copies and
    derivatives.

    This software is supplied "as is" without express or implied warranty.

    But that said, if there are any problems please get in touch.
*/

#ifndef WIN_DIRENT_INCLUDED
#define WIN_DIRENT_INCLUDED

/*
    Declaration of POSIX directory browsing functions and types for Win32.

    Author:  Kevlin Henney (kevlin@acm.org, kevlin@curbralan.com)
    History: Created March 1997. Updated June 2003.
    Rights:  See end of file.
*/

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct win_DIR win_DIR;

struct win_dirent
{
    char *d_name;
};

win_DIR           *win_opendir(const char *);
int			       win_closedir(win_DIR *);
struct win_dirent *win_readdir(win_DIR *);
void               win_rewinddir(win_DIR *);



#ifdef __cplusplus
}
#endif

#endif

// VTK-HeaderTest-Exclude: win_windirent.h
