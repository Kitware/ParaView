#ifndef PyMPI_COMPAT_SGIMPI_H
#define PyMPI_COMPAT_SGIMPI_H

/* ---------------------------------------------------------------- */

#undef  MPI_ARGV_NULL
#define MPI_ARGV_NULL ((char **)0)

#undef  MPI_ARGVS_NULL
#define MPI_ARGVS_NULL ((char ***)0)

#undef  MPI_ERRCODES_IGNORE
#define MPI_ERRCODES_IGNORE ((int *)0)

/* ---------------------------------------------------------------- */

#endif /* !PyMPI_COMPAT_SGIMPI_H */
