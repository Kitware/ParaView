/* Id
 *
 * spy_file.c - collection of routines for i/o visualization data to/from disk
 *

    Author : David Crawford

    Date   : 08/30/2000

    Date     |   Modification
    =========|==============================================================

   ***********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "stm_types.h"
#include "spy_file.h"
#include "structured_mesh.h"

#define FALSE 0
#define TRUE  1


/*-------------------------------------------------------------------------*/

static void spy_clean_structured_mesh_data(Structured_Mesh_Data* stm_ptmp);
static void spy_clean_structured_mesh_data_mfield(Structured_Mesh_Data* stm_ptmp);
static void spy_clean_structured_mesh_data_cfield(Structured_Mesh_Data* stm_ptmp);

/* external procedure used to reallocate block memory
    (found in stm_init.c) */
static void realloc_blocks(Structured_Mesh_Data* stm, int n_blocks);
static void check_endian(SpyFile* spy);
static double flt2dbl(SpyFile* spy, unsigned char *a);
static int int4_2_int(SpyFile* spy, unsigned char *a);

static void rld(SpyFile* spy, double *kplane, int n, void *in, int n_in);
static void rld_trend(SpyFile* spy, double *kplane, int n, void *in, int n_in);
static void rld_int(SpyFile* spy, int *data, int n, void *in, int n_in);

static void read_dump_header(SpyFile* spy);
static int read_group_header(SpyFile* spy);
static void read_groups(SpyFile* spy);
static int read_file_header(SpyFile* spy);
static void read_histogram_data(SpyFile* spy);
static void read_block_geometries(SpyFile* spy);
static void read_tracers(SpyFile* spy);

/* Should make these macros */
static double max3(double x, double y, double z);

/* Routine flips an array of ints */
static void flip_int(int *a, int nint);

/* Routine flips an array of doubles */
static void flip_double(double *a, int ndouble);

/* Routine fread's an array of int's in a machine-independent manner */
static void fread_int(SpyFile* spy, int *a, int count, FILE *fp);

/* Routine fread's an array of double's in a machine-independent manner */
static void fread_double(SpyFile* spy, double *a, int count, FILE *fp);
static void ClearDumps(SpyFile* spy);
static void NewDumpR(SpyFile* spy, int cycle, double time, double offset);
static void stm_free_block(Structured_Mesh_Data* stm_data, int block);
static void init_group_header(SpyFile* spy);

/*-------------------------------------------------------------------------*/
SpyFile* spy_initialize()
{
  SpyFile* spy = (SpyFile*) malloc(sizeof(SpyFile));
  memset(spy, 0, sizeof(SpyFile));

  /* List of commands to be executed as a script */
  spy->stm_command = NULL;
  spy->num_stm_commands=0;

  spy->next_time = 0;
  spy->present_time = -1;
  spy->last_time = -1;
  spy->present_dt = 0;

  spy->next_cycle = -1;
  spy->present_cycle = 0;
  spy->present_block=0;

  /* Flag that describes the byte order of doubles, ints, etc. */
  spy->intel_way=0; /* 1 = ordering for Intel processors,
                       0 = ordering for everyone else */

  /* of variables to be saved */

  spy->NumSavedVariables=0;
  spy->SavedVariables = NULL;
  spy->SavedVariablesOffset = NULL;

  spy->in_file=NULL;
  spy->out_file=NULL;

  /* File header information */
  strcpy(spy->SpyFileTitle,"Spymaster output file");
  spy->SpyFileCurrent=101;
  spy->SpyFileVersion=0;
  spy->SpyCompression=0; /* 0=rle, 1=gzip/gunzip? */
  spy->LNDumps=0;
  spy->ndumps=0;
  spy->ThisDump = NULL;
  spy->FirstDump = NULL;
  spy->LastOffset=0;
  spy->FirstGroup=0;
  spy->LastGroup=0;

  return spy;
}

/*-------------------------------------------------------------------------*/
void spy_finalize(SpyFile* spy)
{
  SpyFileDump* curr = 0;
  SpyFileDump* next = 0;
  spy_clean_structured_mesh_data(&spy->stm_data);
  if (spy->NumSavedVariables>0)
    {
    free(spy->SavedVariables);
    spy->SavedVariables = 0;
    free(spy->SavedVariablesOffset);
    spy->SavedVariablesOffset = 0;
    free(spy->SavedVariablesValid);
    spy->SavedVariablesValid = 0;
    }
  for ( curr = spy->FirstDump; curr; curr = next )
    {
    next = curr->next;
    free(curr);
    }
  if ( spy->in_file )
    {
    fclose(spy->in_file);
    spy->in_file = 0;
    }
  spy_setfilename(spy, 0);
  
  free(spy);
}

/*-------------------------------------------------------------------------*/
static void spy_clean_structured_mesh_data_cfield(Structured_Mesh_Data* stm_ptmp)
{
  int i;
  if ( stm_ptmp->CField_id )
    {
    for ( i = 0; i < stm_ptmp->NCFields; i ++ )
      {
      free(stm_ptmp->CField_id[i]);
      }
    free(stm_ptmp->CField_id);
    stm_ptmp->CField_id = 0;
    }
  if ( stm_ptmp->CField_comment)
    {
    for ( i = 0; i < stm_ptmp->NCFields; i ++ )
      {
      free(stm_ptmp->CField_comment[i]);
      }
    free(stm_ptmp->CField_comment);
    stm_ptmp->CField_comment = 0;
    }
  if ( stm_ptmp->CField_int)
    {
    free(stm_ptmp->CField_int);
    stm_ptmp->CField_int = 0;
    }
}

/*-------------------------------------------------------------------------*/
static void spy_clean_structured_mesh_data_mfield(Structured_Mesh_Data* stm_ptmp)
{
  int i;
  if ( stm_ptmp->MField_id )
    {
    for ( i = 0; i < stm_ptmp->NMFields; i ++ )
      {
      free(stm_ptmp->MField_id[i]);
      }
    free(stm_ptmp->MField_id);
    stm_ptmp->MField_id = 0;
    }
  if ( stm_ptmp->MField_comment)
    {
    for ( i = 0; i < stm_ptmp->NMFields; i ++ )
      {
      free(stm_ptmp->MField_comment[i]);
      }
    free(stm_ptmp->MField_comment);
    stm_ptmp->MField_comment = 0;
    }
  if ( stm_ptmp->MField_int)
    {
    free(stm_ptmp->MField_int);
    stm_ptmp->MField_int = 0;
    }
}

/*-------------------------------------------------------------------------*/
static void spy_clean_structured_mesh_data(Structured_Mesh_Data* stm_ptmp)
{
  spy_clean_structured_mesh_data_cfield(stm_ptmp);
  spy_clean_structured_mesh_data_mfield(stm_ptmp);
  realloc_blocks(stm_ptmp, 0);
  if (stm_ptmp->XTracer) { free(stm_ptmp->XTracer); }
  if (stm_ptmp->YTracer) { free(stm_ptmp->YTracer); }
  if (stm_ptmp->ZTracer) { free(stm_ptmp->ZTracer); }
  if (stm_ptmp->LTracer) { free(stm_ptmp->LTracer); }
  if (stm_ptmp->ITracer) { free(stm_ptmp->ITracer); }
  if (stm_ptmp->JTracer) { free(stm_ptmp->JTracer); }
  if (stm_ptmp->KTracer) { free(stm_ptmp->KTracer); }
  memset(stm_ptmp, 0, sizeof(Structured_Mesh_Data));
}

/* Set the filename of the file to read */
void spy_setfilename(SpyFile* spy, const char *filename)
{
  if ( spy->SavedVariablesFileName )
    {
    free(spy->SavedVariablesFileName);
    spy->SavedVariablesFileName = 0;
    }
  if ( filename )
    {
    spy->SavedVariablesFileName = (char*) malloc( sizeof(char) * strlen(filename) + 1 );
    strcpy(spy->SavedVariablesFileName, filename);
    }
}


/* Should make these macros */

double max3(double x, double y, double z)
{
  if (x>y && x>z) return x;
  if (y>x && y>z) return y;
  return z;
}

void stm_free_block(Structured_Mesh_Data* stm_data, int block)
{
  Structured_Block_Data *blk;
  double ***field;
  int k,l,m,Nz;

  blk = &stm_data->block[block];
  if (blk == NULL) return;

  Nz=blk->Nz;

  /* Free the cell field data. */

  if (blk->CField != NULL)
    {
    for (l = 0; l < stm_data->NCFields; l++)
      {
      field = blk->CField[l];
      if (field != NULL)
        {
        for (k = 0; k < Nz; k++)
          {
          if (field[k] != NULL)
            {
            if (field[k][0] != NULL)
              {
              /* free the entire k-plane...which is the way it was allocated */
              free(field[k][0]);
              }
            free(field[k]);
            }
          }
        free(field);
        }
      }
    free(blk->CField);
    }

  if (blk->MField != NULL)
    {
    for (l = 0; l < stm_data->NMFields; l++)
      {
      for (m =0; m < stm_data->Nmat; m++)
        {
        field = blk->MField[l][m];
        if (field != NULL)
          {
          for (k = 0; k < Nz; k++)
            {
            if (field[k] != NULL)
              {
              if (field[k][0] != NULL)
                {
                /* free the entire k-plane...which is the way it was allocated */
                free(field[k][0]);
                }
              free(field[k]);
              }
            }
          free(field);
          }
        }
      if (blk->MField[l] != NULL) free(blk->MField[l]);
      }
    free(blk->MField);
    }

  if (blk->x != NULL) free(blk->x);
  if (blk->y != NULL) free(blk->y);
  if (blk->z != NULL) free(blk->z);
}

void realloc_blocks(Structured_Mesh_Data* stm_data, int n_blocks)
{
  int i;

  /*  Commented the following out to force re-initialization of
      all block data every time a new file is read. This was
      causing a seg fault when working with mpcth data that
      has only 1 block per processor but slight variance in
      the geometry (# cells in x,y or z change slightly from
      processor to processor. */
  if (n_blocks && (n_blocks <= stm_data->Nblocks))
    {
    /* printf("Reused block memory\n"); */
    return;
    }

  /* printf("Freeing and allocating block memory!\n"); */
  /* printf("Allocating %d blocks...\n",n_blocks); */
 
 
  /* Free the old block array */
  for (i=0; i<stm_data->Nblocks; i++)
    {
    stm_free_block(stm_data, i);
    }
  if ( stm_data->block )
    {
    free(stm_data->block);
    }
  stm_data->block=NULL;
  if ( n_blocks <= 0 )
    {
    return;
    }

  /* Allocate the new block array */
  stm_data->Nblocks = n_blocks;
  stm_data->block =
    (Structured_Block_Data *) malloc(n_blocks * sizeof(Structured_Block_Data));

  /* Initialize the block array */
  for (i = 0; i < stm_data->Nblocks; i++)
    {
    stm_data->block[i].allocated = 0;
    stm_data->block[i].active = 0;
    stm_data->block[i].CField = NULL;
    stm_data->block[i].MField = NULL;
    stm_data->block[i].Nx=0;
    stm_data->block[i].Ny=0;
    stm_data->block[i].Nz=0;
    stm_data->block[i].x=NULL;
    stm_data->block[i].y=NULL;
    stm_data->block[i].z=NULL;
    }
}



double *** spy_GetField(Structured_Block_Data *blk, int field_id)
{
  int tmp,tmp1;

  /* Material field */

  if (field_id>=100)
    {
    if (blk->MField==NULL)
      {
      return NULL;
      }
    tmp=field_id/100;
    tmp--;
    if (blk->MField[tmp]==NULL)
      {
      return NULL;
      }
    tmp1=field_id-(tmp+1)*100;
    tmp1--;
    return blk->MField[tmp][tmp1];   
    }
  else
    {
    /* Cell field */
    if (blk->CField==NULL)
      {
      return NULL;
      }
    return blk->CField[field_id];
    }
}

/* Routine flips an array of ints */
void flip_int(int *a, int nint)
{
  register int i;
  int tmp;
  unsigned char *pa, *ptmp;

  ptmp=(unsigned char *) &tmp;
  for (i=0; i<nint; i++)
    {
    tmp=a[i];
    pa = (unsigned char *) &a[i];
    pa[0] = ptmp[3];
    pa[1] = ptmp[2];
    pa[2] = ptmp[1];
    pa[3] = ptmp[0];
    }
}

/* Routine flips an array of doubles */
void flip_double(double *a, int ndouble)
{
  register int i;
  double tmp;
  unsigned char *pa, *ptmp;

  ptmp=(unsigned char *) &tmp;
  for (i=0; i<ndouble; i++)
    {
    tmp=a[i];
    pa = (unsigned char *) &a[i];
    pa[0] = ptmp[7];
    pa[1] = ptmp[6];
    pa[2] = ptmp[5];
    pa[3] = ptmp[4];
    pa[4] = ptmp[3];
    pa[5] = ptmp[2];
    pa[6] = ptmp[1];
    pa[7] = ptmp[0];
    }
}




/* Routine fread's an array of int's in a machine-independent manner */
void fread_int(SpyFile* spy, int *a, int count, FILE *fp)
{
  int in_count;
  in_count = fread(a,sizeof(int),count,fp);
  if (spy->intel_way)
    {
    flip_int(a,count);
    }
}

/* Routine fread's an array of double's in a machine-independent manner */
void fread_double(SpyFile* spy, double *a, int count, FILE *fp)
{
  fread(a,sizeof(double),count,fp);
  if (spy->intel_way)
    {
    flip_double(a,count);
    }
}

/* Routine fread's an array of double's in a machine-independent manner and convert to long*/
void fread_offsets(SpyFile* spy, long *a, int count, FILE *fp)
{
  int i;
  double* buffer;

  /* Allocate temporary buffer */
  buffer = (double*) malloc (sizeof(double)*count);

  /* Use existing double function to read in data */
  fread_double(spy,buffer,count,fp);

  /* okay now convert to longs */
  for(i=0;i<count;++i)
    {
    a[i] = (long)buffer[i];
    }

  /* Free the temporary buffer */
  free(buffer);
}

/* Global variables describing the spy file */

/* Routine initializes the group header to zeros */
void init_group_header(SpyFile* spy)
{
  int i;

  for (i=0; i<MAX_DUMPS; i++)
    {
    spy->DumpCycle[i]=0;
    spy->DumpTime[i]=0;
    spy->DumpOffset[i]=0;
    } 
}



int read_group_header(SpyFile* spy)
{
  int ntmp;
  long lg = (long)spy->LastGroup;

  if (spy->in_file == NULL) return 0;

  /* Position the file pointer to spy->LastGroup */
  fseek(spy->in_file,lg,SEEK_SET);

  /* This stuff is needed for each section of MAX_DUMPS dumps */
  fread_int(spy, &ntmp,1,spy->in_file);
  fread_int(spy, spy->DumpCycle,MAX_DUMPS,spy->in_file);
  fread_double(spy, spy->DumpTime,MAX_DUMPS,spy->in_file);
  fread_double(spy, spy->DumpOffset,MAX_DUMPS,spy->in_file);
  fread_double(spy, &spy->LastOffset,1,spy->in_file);
  return ntmp;
}

void ClearDumps(SpyFile* spy)
{
  SpyFileDump *tmp;

  spy->ThisDump=spy->FirstDump;
  while (spy->ThisDump != NULL)
    {
    tmp = spy->ThisDump;
    spy->ThisDump=(SpyFileDump*)spy->ThisDump->next;
    free(tmp);
    }
  spy->FirstDump=spy->ThisDump=NULL;
}


void NewDumpR(SpyFile* spy, int cycle, double time, double offset)
{
  if (spy->FirstDump==NULL)
    {
    spy->FirstDump = (SpyFileDump *) malloc(sizeof(SpyFileDump));
    spy->ThisDump=spy->FirstDump;
    }
  else
    {
    spy->ThisDump->next = (SpyFileDump *) malloc(sizeof(SpyFileDump));
    spy->ThisDump=(SpyFileDump*)spy->ThisDump->next;
    }
  spy->ThisDump->Cycle=cycle;
  spy->ThisDump->Time=time;
  spy->ThisDump->Offset=offset;
  spy->ThisDump->next=NULL;
}

void read_groups(SpyFile* spy)
{
  int n;

  ClearDumps(spy);
  spy->LNDumps=0;
  spy->LastGroup = spy->FirstGroup;
  do
    {
    spy->ndumps = read_group_header(spy);
    for (n=0; n<spy->ndumps; n++)
      {
      NewDumpR(spy, spy->DumpCycle[n],spy->DumpTime[n],spy->DumpOffset[n]);
      }
    spy->LNDumps += spy->ndumps;
    if (spy->ndumps == MAX_DUMPS)
      {
      spy->LastGroup=spy->LastOffset;
      }
    }
  while (spy->ndumps == MAX_DUMPS);
}



/* Routine reads the file header information */
int read_file_header(SpyFile* spy)
{
  int i, n_blocks, n_tmp;
  char magic[8];
  Structured_Mesh_Data *stm_ptmp;
  int not_used;

  if (spy->in_file==NULL)
    {
    return 1;
    }
  if (spy->FirstDump== NULL)
    {
    init_group_header(spy);
    }
  rewind(spy->in_file);
  /* Check magic string "spydata" */
  fread(magic,sizeof(char),8,spy->in_file);
  if (strcmp("spydata",magic)!=0)
    {
    fclose(spy->in_file);
    spy->in_file = 0;
    return 1;
    }

  /* If not running on-the-fly, we assume the data in the file header
     is correct and replace the contents of stm_data with that from
     the file. */
  stm_ptmp = &spy->stm_data;

  fread(spy->SpyFileTitle,sizeof(char),128,spy->in_file);
  fread_int(spy, &spy->SpyFileVersion,1,spy->in_file);
  /* Force error return if earlier file version */
  if (spy->SpyFileVersion<100)
    {
    fprintf(stderr,"Error: this is an unsupported file version: %4.2f\n",spy->SpyFileVersion/100.);
    fprintf(stderr,"       (the current version is %4.2f)\n",spy->SpyFileCurrent/100.);
    fclose(spy->in_file);
    spy->in_file = 0;
    return 1;
    }
  /* Print warning message if earlier file version */
  if (spy->SpyFileVersion!=spy->SpyFileCurrent)
    {
    fprintf(stderr,"Warning: this is an earlier file version: %4.2f\n",spy->SpyFileVersion/100.);
    fprintf(stderr,"         (the current version is %4.2f)\n",spy->SpyFileCurrent/100.);
    fprintf(stderr,"         some features may not be supported\n");
    }
  fread_int(spy, &spy->SpyCompression,1,spy->in_file);
  fread_int(spy, &not_used,1,spy->in_file);
  fread_int(spy, &not_used,1,spy->in_file);
  fread_int(spy, &stm_ptmp->IGM,1,spy->in_file);
  fread_int(spy, &stm_ptmp->Ndim,1,spy->in_file);
  fread_int(spy, &stm_ptmp->Nmat,1,spy->in_file);
  fread_int(spy, &stm_ptmp->MaxMat,1,spy->in_file);
  fread_double(spy, stm_ptmp->Gmin,3,spy->in_file);
  fread_double(spy, stm_ptmp->Gmax,3,spy->in_file);
  fread_int(spy, &n_blocks,1,spy->in_file);
  fread_int(spy, &stm_ptmp->max_level,1,spy->in_file);

  /* allocate memory (if necessary) for cell field id's and ints */
  fread_int(spy, &n_tmp,1,spy->in_file);
  if (stm_ptmp->NCFields==0)
    {
    spy_clean_structured_mesh_data_cfield(stm_ptmp);
    stm_ptmp->NCFields=n_tmp;
    stm_ptmp->CField_id = (char **) malloc(sizeof(char *)*stm_ptmp->NCFields);
    stm_ptmp->CField_comment = (char **) malloc(sizeof(char *)*stm_ptmp->NCFields);
    stm_ptmp->CField_int = (int *) malloc(sizeof(int)*stm_ptmp->NCFields);
    for (i=0; i<stm_ptmp->NCFields; i++)
      {
      stm_ptmp->CField_id[i] = (char *) malloc(sizeof(char)*30);
      stm_ptmp->CField_comment[i] = (char *) malloc(sizeof(char)*80);
      }
    }
  /* read in cell field id's and ints */
  for (i=0; i<stm_ptmp->NCFields; i++)
    {
    fread(stm_ptmp->CField_id[i],sizeof(char),30,spy->in_file);
    fread(stm_ptmp->CField_comment[i],sizeof(char),80,spy->in_file);
    if (spy->SpyFileVersion>=101)
      {
      fread_int(spy, &stm_ptmp->CField_int[i],1,spy->in_file);
      }
    else
      {
      stm_ptmp->CField_int[i]=i;
      }
    }

  /* allocate memory (if necessary) for material field id's and ints */
  fread_int(spy, &n_tmp,1,spy->in_file);
  if (stm_ptmp->NMFields==0)
    {
    spy_clean_structured_mesh_data_mfield(stm_ptmp);
    stm_ptmp->NMFields=n_tmp;
    stm_ptmp->MField_id = (char **) malloc(sizeof(char *)*stm_ptmp->NMFields);
    stm_ptmp->MField_comment = (char **) malloc(sizeof(char *)*stm_ptmp->NMFields);
    stm_ptmp->MField_int = (int *) malloc(sizeof(int)*stm_ptmp->NMFields);
    for (i=0; i<stm_ptmp->NMFields; i++)
      {
      stm_ptmp->MField_id[i] = (char *) malloc(sizeof(char)*30);
      stm_ptmp->MField_comment[i] = (char *) malloc(sizeof(char)*80);
      }
    }

  /* read in material field id's and ints */
  for (i=0; i<stm_ptmp->NMFields; i++)
    {
    fread(stm_ptmp->MField_id[i],sizeof(char),30,spy->in_file);
    fread(stm_ptmp->MField_comment[i],sizeof(char),80,spy->in_file);
    if (spy->SpyFileVersion>=101)
      {
      fread_int(spy, &stm_ptmp->MField_int[i],1,spy->in_file);
      }
    else 
      {
      stm_ptmp->MField_int[i] = (i+1)*100;
      }
    }

  /* Read in the file offset */
  fread_double(spy, &spy->FirstGroup,1,spy->in_file);
  read_groups(spy);
  return 0;
}

/* Routine opens a spy visualization file for input and
   reads the header information. */
int spy_open_file_for_input(SpyFile* spy, const char *filename)
{
  int error;

  /* If I have an open file close it */
  if (spy->in_file != NULL)
    {
    fclose(spy->in_file);
    spy->in_file = 0;
    }

  /* Open the file */
  spy->in_file = fopen(filename,"rb");
  if (spy->in_file==NULL) 
    {
    fprintf(stderr,"Error: could not open %s\n",filename);
    return 1;
    }

  spy_setfilename(spy, filename);

  check_endian(spy);
  error = read_file_header(spy);

  if (error) 
    {
    fprintf(stderr,"Error: reading file %s\n",filename);
    return 1;
    }

  return 0;
}



double flt2dbl(SpyFile* spy, unsigned char *a)
{
  unsigned char *ptmp;

  /* The following 2 lines should be ifdef'd for different machines */
  float tmp;
  ptmp = (unsigned char *) &tmp;

  if (spy->intel_way)
    {
    ptmp[0]=a[3];
    ptmp[1]=a[2];
    ptmp[2]=a[1];
    ptmp[3]=a[0];
    } else
    {
    ptmp[0]=a[0];
    ptmp[1]=a[1];
    ptmp[2]=a[2];
    ptmp[3]=a[3];
    }

  return (double) tmp;
}


int int4_2_int(SpyFile* spy, unsigned char *a)
{
  unsigned char *ptmp;

  /* The following 2 lines should be ifdef'd for different machines */
  int tmp;
  ptmp = (unsigned char *) &tmp;

  if (spy->intel_way)
    {
    ptmp[0]=a[3];
    ptmp[1]=a[2];
    ptmp[2]=a[1];
    ptmp[3]=a[0];
    } else
    {
    ptmp[0]=a[0];
    ptmp[1]=a[1];
    ptmp[2]=a[2];
    ptmp[3]=a[3];
    }

  return (int) tmp;
}

/* Routine run-length-encodes the data pointed to by *data, placing
   the result in *out. n is the number of doubles to encode. n_out
   is the number of bytes used for the compression (and stored at
 *out). delta is the smallest change of adjacent values that will
 be accepted (changes smaller than delta will be ignored). 

Note: *out needs to be allocated by the calling application. 
Its worst-case size is 5*n bytes. */



/* Routine run-length-decodes the data pointed to by *in and
   returns a collection of doubles in *data. Performs the
   inverse of rle above. Application should provide both
   n (the expected number of doubles) and n_in the number
   of bytes to decode from *in. Again, the application needs
   to provide allocated space for *data which will be
   n bytes long. */

void rld(SpyFile* spy, double *data, int n, void *in, int n_in)
{
  int i,j,k;
  unsigned char *ptmp,code;
  double x;

  ptmp=in;

  /* Run-length decode */

  i=0;
  j=0;
  while ((i<n) && (j<n_in))
    {
    code=*ptmp;
    ptmp++;
    if (code<128)
      {
      x=flt2dbl(spy, ptmp);
      ptmp+=4;
      for (k=0; k<code; k++)
        {
        data[i++]=x;
        }
      j+=5;
      }
    else
      {
      for (k=0; k<code-128; k++)
        {
        data[i++]=flt2dbl(spy, ptmp);
        ptmp+=4;
        }
      j+=4*(code-128)+1;
      }
    }
}


void rld_trend(SpyFile* spy, double *data, int n, void *in, int n_in)
{
  int i,j,k;
  unsigned char *ptmp,code;
  double x,dx;

  ptmp=in;

  /* Run-length decode */

  i=0;
  j=0;
  x=flt2dbl(spy, ptmp);
  ptmp+=4;
  dx=flt2dbl(spy, ptmp);
  ptmp+=4;
  j+=8;
  while ((i<n) && (j<n_in))
    {
    code=*ptmp;
    ptmp++;
    if (code<128)
      {
      ptmp+=4;
      for (k=0; k<code; k++)
        {
        data[i]=x+i*dx;
        i++;
        }
      j+=5;
      } else
      {
      for (k=0; k<code-128; k++)
        {
        data[i]=flt2dbl(spy, ptmp)+i*dx;
        i++;
        ptmp+=4;
        }
      j+=4*(code-128)+1;
      }
    }
}



/* run-length-decode the integer data pointed to by *data
   Inverse of rle_int above */
void rld_int(SpyFile* spy, int *data, int n, void *in, int n_in)
{
  int i,j,k;
  unsigned char *ptmp,code;
  int x;

  ptmp=in;

  /* Run-length decode */

  i=0;
  j=0;
  while ((i<n) && (j<n_in))
    {
    code=*ptmp;
    ptmp++;
    if (code<128)
      {
      x=int4_2_int(spy, ptmp);
      ptmp+=4;
      for (k=0; k<code; k++) data[i++]=x;
      j+=5;
      } else
      {
      for (k=0; k<code-128; k++)
        {
        data[i++]=int4_2_int(spy, ptmp);
        ptmp+=4;
        }
      j+=4*(code-128)+1;
      }
    }
}

void read_dump_header(SpyFile* spy)
{
  int tmp;

  /* Input the number of saved variables, their id's and offsets */
  fread_int(spy, &tmp,1,spy->in_file);
  if (tmp != spy->NumSavedVariables)
    {
    if (spy->NumSavedVariables>0)
      {
      free(spy->SavedVariables);
      free(spy->SavedVariablesOffset);
      free(spy->SavedVariablesValid);
      }
    spy->NumSavedVariables = tmp;
    spy->SavedVariables = (int *) malloc(spy->NumSavedVariables*sizeof(int));
    spy->SavedVariablesOffset = (double *) malloc(spy->NumSavedVariables*sizeof(double));
    spy->SavedVariablesValid = (char*) malloc(spy->NumSavedVariables*sizeof(char));
    memset(spy->SavedVariablesValid, 0, spy->NumSavedVariables * sizeof(char));
    }
  fread_int(spy, spy->SavedVariables,spy->NumSavedVariables,spy->in_file);
  fread_double(spy, spy->SavedVariablesOffset,spy->NumSavedVariables,spy->in_file);
}


/* Input the histogram data */
void read_histogram_data(SpyFile* spy)
{
  int i;
  unsigned char *buffer;
  int n_bytes_in,num_indicators;

  /* read the number of indicators in this dump */
  fread_int(spy, &num_indicators,1,spy->in_file);
  if (num_indicators>0)
    {
    fread_int(spy, &spy->stm_data.MaxBin,1,spy->in_file);
    if (num_indicators!=spy->stm_data.NIndicators)
      {
      if (spy->stm_data.NIndicators>0)
        {
        free(spy->stm_data.HistMin);
        free(spy->stm_data.HistMax);
        free(spy->stm_data.HistType);
        free(spy->stm_data.RefAbove);
        free(spy->stm_data.RefBelow);
        free(spy->stm_data.UnrAbove);
        free(spy->stm_data.UnrBelow);
        for (i=0; i<spy->stm_data.NIndicators; i++)
          {
          free(spy->stm_data.Histogram[i]);
          }
        free(spy->stm_data.Histogram);
        free(spy->stm_data.NBins);
        }
      spy->stm_data.Histogram = (double **) malloc(sizeof(double *)*num_indicators);
      spy->stm_data.HistMin = (double *) malloc(sizeof(double)*num_indicators);
      spy->stm_data.HistMax = (double *) malloc(sizeof(double)*num_indicators);
      spy->stm_data.HistType = (int *) malloc(sizeof(int)*num_indicators);
      spy->stm_data.RefAbove = (double *) malloc(sizeof(double)*num_indicators);
      spy->stm_data.RefBelow = (double *) malloc(sizeof(double)*num_indicators);
      spy->stm_data.UnrAbove = (double *) malloc(sizeof(double)*num_indicators);
      spy->stm_data.UnrBelow = (double *) malloc(sizeof(double)*num_indicators);
      spy->stm_data.NBins = (int *) malloc(sizeof(int)*num_indicators);
      for (i=0; i<num_indicators; i++)
        {
        spy->stm_data.Histogram[i] = NULL;
        }
      }
    /* loop over the indicators and input...*/
    for (i=0; i<num_indicators; i++)
      {
      fread_int(spy, &spy->stm_data.HistType[i],1,spy->in_file);
      fread_double(spy, &spy->stm_data.RefAbove[i],1,spy->in_file);
      fread_double(spy, &spy->stm_data.RefBelow[i],1,spy->in_file);
      fread_double(spy, &spy->stm_data.UnrAbove[i],1,spy->in_file);
      fread_double(spy, &spy->stm_data.UnrBelow[i],1,spy->in_file);
      fread_double(spy, &spy->stm_data.HistMin[i],1,spy->in_file);
      fread_double(spy, &spy->stm_data.HistMax[i],1,spy->in_file);
      fread_int(spy, &spy->stm_data.NBins[i],1,spy->in_file);
      if (spy->stm_data.NBins[i]>0)
        {
        spy->stm_data.Histogram[i] = (double *) malloc(spy->stm_data.NBins[i]*sizeof(double));
        buffer = (unsigned char *) malloc(5*spy->stm_data.NBins[i]+8);
        fread_int(spy, &n_bytes_in,1,spy->in_file);
        fread(buffer,1,n_bytes_in,spy->in_file);
        rld(spy, spy->stm_data.Histogram[i],spy->stm_data.NBins[i],buffer,n_bytes_in);
        free(buffer);
        }
      }
    }
  spy->stm_data.NIndicators = num_indicators;
}



/* Input the number of blocks and the block geometries */
void read_block_geometries(SpyFile* spy)
{
  int j,k,l,m,n;
  int Ny, Nz;
  Structured_Block_Data *blk;
  unsigned char *buffer=NULL;
  int n_bytes_in,tmp,max,n_blocks;
  double ***field;

  /* read in the number of blocks in this dump */
  fread_int(spy, &n_blocks,1,spy->in_file);

  /* realloc block arrays, if necessary */
  realloc_blocks(&spy->stm_data, n_blocks);

  /* loop over the blocks and input the logical dimensions
     (Nx, Ny, Nz) of the mesh and the active block flags... */
  max=0;
  for (n=0; n<spy->stm_data.Nblocks; n++)
    {
    blk = &spy->stm_data.block[n];
    fread_int(spy, &blk->Nx,1,spy->in_file);
    fread_int(spy, &blk->Ny,1,spy->in_file);
    fread_int(spy, &blk->Nz,1,spy->in_file);
    fread_int(spy, &blk->allocated,1,spy->in_file);
    fread_int(spy, &blk->active,1,spy->in_file);
    fread_int(spy, &blk->level,1,spy->in_file);
    tmp=(int)(5*max3(blk->Nx+1,blk->Ny+1,blk->Nz+1)+8);
    if (tmp>max) max=tmp;
    /* allocate space for the geometry arrays, if needed */
    if (blk->x == NULL) 
      blk->x = (double *) malloc((blk->Nx+1)*sizeof(double));
    if (blk->y == NULL) 
      blk->y = (double *) malloc((blk->Ny+1)*sizeof(double));
    if (blk->z == NULL) 
      blk->z = (double *) malloc((blk->Nz+1)*sizeof(double));
    }
  buffer=(unsigned char *) malloc(max);
  /* loop over the blocks read and uncompress the geometries
     of allocated blocks */
  for (n=0; n<spy->stm_data.Nblocks; n++)
    {
    blk = &spy->stm_data.block[n];
    if(blk->allocated)
      {
      fread_int(spy, &n_bytes_in,1,spy->in_file);
      fread(buffer,1,n_bytes_in,spy->in_file);
      rld_trend(spy, blk->x,blk->Nx+1,buffer,n_bytes_in);
      fread_int(spy, &n_bytes_in,1,spy->in_file);
      fread(buffer,1,n_bytes_in,spy->in_file);
      rld_trend(spy, blk->y,blk->Ny+1,buffer,n_bytes_in);
      fread_int(spy, &n_bytes_in,1,spy->in_file);
      fread(buffer,1,n_bytes_in,spy->in_file);
      rld_trend(spy, blk->z,blk->Nz+1,buffer,n_bytes_in);
      }
    }
  free(buffer);

  /* if needed, allocate space for the variable pointers */
  for (n=0; n<spy->stm_data.Nblocks; n++)
    {
    blk = &spy->stm_data.block[n];
    Ny = blk->Ny;
    Nz = blk->Nz;
    if (blk->allocated)
      {
      if (blk->CField == NULL)
        {
        blk->CField = (double ****) malloc(spy->stm_data.NCFields * sizeof(double *));
        for (l = 0; l < spy->stm_data.NCFields; l++)
          {
          blk->CField[l] = (double ***) malloc(Nz * sizeof(double *));
          field = blk->CField[l];
          for (k = 0; k < Nz; k++)
            {
            field[k] = (double **) malloc(Ny * sizeof(double *));
            for (j = 0; j < Ny; j++)
              {
              field[k][j] = NULL;
              }
            }
          }
        }
      if (blk->MField == NULL)
        {
        blk->MField = (double *****) malloc(spy->stm_data.NMFields * sizeof(double *));
        for (l = 0; l < spy->stm_data.NMFields; l++)
          {
          blk->MField[l] = (double ****) malloc(spy->stm_data.Nmat * sizeof(double *));
          for (m =0; m < spy->stm_data.Nmat; m++)
            {
            blk->MField[l][m] = (double ***) malloc(Nz * sizeof(double *));
            field = blk->MField[l][m];
            for (k = 0; k < Nz; k++)
              {
              field[k] = (double **) malloc(Ny * sizeof(double *));
              for (j = 0; j < Ny; j++)
                {
                field[k][j] = NULL;
                }
              }
            }
          }
        }
      }
    }

}

void read_tracers(SpyFile* spy)
{
  int old_tracerN;
  unsigned char *buffer;
  int n_bytes_in;

  /* read the number of tracers in this dump */
  old_tracerN = spy->stm_data.NTracers; 
  fread_int(spy, &spy->stm_data.NTracers,1,spy->in_file);

  if (spy->stm_data.NTracers>0)
    {

    /* Allocate space for the tracer arrays, if needed */

    if (spy->stm_data.NTracers > old_tracerN)
      {
      if (spy->stm_data.XTracer) { free(spy->stm_data.XTracer); }
      if (spy->stm_data.YTracer) { free(spy->stm_data.YTracer); }
      if (spy->stm_data.ZTracer) { free(spy->stm_data.ZTracer); }
      if (spy->stm_data.LTracer) { free(spy->stm_data.LTracer); }
      if (spy->stm_data.ITracer) { free(spy->stm_data.ITracer); }
      if (spy->stm_data.JTracer) { free(spy->stm_data.JTracer); }
      if (spy->stm_data.KTracer) { free(spy->stm_data.KTracer); }
      spy->stm_data.XTracer = (double *) malloc(sizeof(double)*spy->stm_data.NTracers);
      spy->stm_data.YTracer = (double *) malloc(sizeof(double)*spy->stm_data.NTracers);
      spy->stm_data.ZTracer = (double *) malloc(sizeof(double)*spy->stm_data.NTracers);
      spy->stm_data.LTracer = (int *) malloc(sizeof(double)*spy->stm_data.NTracers);
      spy->stm_data.ITracer = (int *) malloc(sizeof(double)*spy->stm_data.NTracers);
      spy->stm_data.JTracer = (int *) malloc(sizeof(double)*spy->stm_data.NTracers);
      spy->stm_data.KTracer = (int *) malloc(sizeof(double)*spy->stm_data.NTracers);
      }

    /* Allocate space for a temporary buffer */
    buffer = (unsigned char *) malloc(5*spy->stm_data.NTracers+8);

    fread_int(spy, &n_bytes_in,1,spy->in_file);
    fread(buffer,1,n_bytes_in,spy->in_file);
    rld(spy, spy->stm_data.XTracer,spy->stm_data.NTracers,buffer,n_bytes_in);

    fread_int(spy, &n_bytes_in,1,spy->in_file);
    fread(buffer,1,n_bytes_in,spy->in_file);
    rld(spy, spy->stm_data.YTracer,spy->stm_data.NTracers,buffer,n_bytes_in);

    fread_int(spy, &n_bytes_in,1,spy->in_file);
    fread(buffer,1,n_bytes_in,spy->in_file);
    rld(spy, spy->stm_data.ZTracer,spy->stm_data.NTracers,buffer,n_bytes_in);

    fread_int(spy, &n_bytes_in,1,spy->in_file);
    fread(buffer,1,n_bytes_in,spy->in_file);
    rld_int(spy, spy->stm_data.LTracer,spy->stm_data.NTracers,buffer,n_bytes_in);

    fread_int(spy, &n_bytes_in,1,spy->in_file);
    fread(buffer,1,n_bytes_in,spy->in_file);
    rld_int(spy, spy->stm_data.ITracer,spy->stm_data.NTracers,buffer,n_bytes_in);

    fread_int(spy, &n_bytes_in,1,spy->in_file);
    fread(buffer,1,n_bytes_in,spy->in_file);
    rld_int(spy, spy->stm_data.JTracer,spy->stm_data.NTracers,buffer,n_bytes_in);

    fread_int(spy, &n_bytes_in,1,spy->in_file);
    fread(buffer,1,n_bytes_in,spy->in_file);
    rld_int(spy, spy->stm_data.KTracer,spy->stm_data.NTracers,buffer,n_bytes_in);

    free(buffer);
    }
}



/* Routine reads variable data for all blocks and kplanes 
   It is assumed that the file pointer is pointing to the
   beginning of the compressed data in file: spy->in_file */
void spy_read_variable_data(SpyFile* spy, int field_index)
{
  int j,k,n;
  Structured_Block_Data *blk;
  unsigned char *buffer=NULL;
  int n_bytes_in,tmp,max;
  double ***field;
  int Variable_id = spy->SavedVariables[field_index];
  if ( spy->SavedVariablesValid[field_index] )
    {
    return;
    }
  /*printf("Read variable data\n");*/

  fseek(spy->in_file,(long)(spy->SavedVariablesOffset[field_index]),SEEK_SET);

  /* loop over the blocks and find the maximum
     required buffer size to hold the compressed variable data. */
  max=0;
  for (n=0; n<spy->stm_data.Nblocks; n++)
    {
    blk = &spy->stm_data.block[n];
    if (blk->allocated)
      {
      tmp=5*blk->Nx*blk->Ny+8;
      if (tmp>max) max=tmp;
      }
    }
  if (max==0) return;

  buffer=(unsigned char *) malloc(max);

  /* loop over the blocks, read and uncompress the data */
  for (n = 0; n < spy->stm_data.Nblocks; n++)
    {
    blk = &spy->stm_data.block[n];
    if (blk->allocated)
      {
      field = spy_GetField(blk,Variable_id);
      for (k=0; k<blk->Nz; k++)
        {
        /* allocate space, if needed */
        if (field[k][0] == NULL)
          {
          /* we allocate an entire kplane...*/
          field[k][0] = (double *) malloc(blk->Nx*blk->Ny*sizeof(double));
          /* and then set the i-strip pointers to point into the kplane */
          for (j=1; j<blk->Ny; j++) field[k][j] = field[k][0] + j*blk->Nx;
          }
        fread_int(spy, &n_bytes_in,1,spy->in_file);
        fread(buffer,1,n_bytes_in,spy->in_file);
        rld(spy, &field[k][0][0], blk->Nx*blk->Ny, buffer, n_bytes_in);
        }

      }
    }
  free(buffer);
  spy->SavedVariablesValid[field_index] = 1;
}

/* Routine that checks endian and sets a global flag */
void check_endian(SpyFile* spy)
{
  float a;
  unsigned char *pa;

  pa = (unsigned char *) &a;
  a = 1.23456e8;
  if (pa[0]==0x4c)
    {
    spy->intel_way=0;
    }
  else
    {
    spy->intel_way=1;
    }
}


/* This function is responsible reading input from an existing spcth
 * file. */
  int
spy_file_in(SpyFile* spy, double time)
  /***********************************************************************

    Routine: spy_file_in

    Author : David Crawford

    Date   : 08/30/2000

    Date     |   Modification
    =========|==============================================================

   ***********************************************************************/
{
  int   n;
  double min_dt;
  SpyFileDump *tmp_dump;
  int n_dt;

  if (spy->LNDumps<1) return 0;

  check_endian(spy);


  /* File is assumed to already be open and the file header
     info. to have been read. */

  /* Find the dump that most closely matches the requested time */
  min_dt = 1e99;
  n=0;
  for (tmp_dump=spy->FirstDump; tmp_dump!=NULL; tmp_dump=(SpyFileDump*)tmp_dump->next)
    {
    if ((tmp_dump->Time>=time) && (tmp_dump->Time-time< min_dt))
      {
      min_dt = tmp_dump->Time-time;
      spy->ThisDump = tmp_dump;
      n_dt=n;
      }
    if (min_dt>1e98) 
      {
      spy->ThisDump=tmp_dump;
      n_dt=n;
      }
    n++;
    }

  /* Check if the same time as last call */
  if (spy->present_time == spy->ThisDump->Time)
    {
    return n_dt;
    }

  /* Okay new time step so invalidate any saved variables */
  memset(spy->SavedVariablesValid, 0, spy->NumSavedVariables * sizeof(char));

  /* Set up local variables based on existing dump structure */
  spy->present_cycle = spy->ThisDump->Cycle;
  spy->present_time = spy->ThisDump->Time;

  /* and position the file pointer */
  fseek(spy->in_file,(int) spy->ThisDump->Offset,SEEK_SET);

  /* These routines read data in the order the data was writtin
   * in the function call to spy_file_out() on a previous run.
   * These functions are defined in the above source code */

  read_dump_header(spy);

  read_tracers(spy);

  read_histogram_data(spy);

  read_block_geometries(spy);

  return n_dt;
}

/* NextBlock and FirstBlock are the routines used to navigate the strips
 * of block data.  Each call to NextBlock will the the next block in the 
 * the strip and read (using spy_file_in() above) all of the data associated
 * with that block */

Structured_Block_Data *spy_NextBlock(SpyFile* spy)
{
  int n;

  for (n=spy->present_block+1; n<spy->stm_data.Nblocks; n++) 
    {
    if (spy->stm_data.block[n].allocated) 
      {
      spy->present_block=n;
      return &spy->stm_data.block[n];
      }
    }
  return NULL;
}

Structured_Block_Data *spy_FirstBlock(SpyFile* spy)
{
  spy->present_block=-1;
  return spy_NextBlock(spy);
}
