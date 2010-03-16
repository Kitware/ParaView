#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <float.h>
#include <queue>
#include <vector>
#include <sys/mman.h>
#include <unistd.h>

using namespace std;

struct point
{
  float x;
  float vx;
  float y;
  float vy;
  float z;
  float vz;
  float mass;
  
  uint64_t tag;
  //int tag;

  point()
  {
  }

  point(const point& p)
  {
    this->x = p.x;
    this->vx = p.vx;
    this->y = p.y;
    this->vy = p.vy;
    this->z = p.z;
    this->vz = p.vz;
    this->mass = p.mass;
    this->tag = p.tag;
  }
};

typedef struct point point;

struct floatfile
{
  float v;
  size_t f;

  floatfile()
  {
  }

  floatfile(const floatfile& ff)
  {
    this->v = ff.v;
    this->f = ff.f;
  }

  bool operator < (const floatfile& ff) const
  {
    return this->v < ff.v;
  }
};

typedef struct floatfile floatfile;

#ifdef __APPLE__
//#define USE_PTHREADS

#include <mach/mach_time.h>
#endif

/*
#ifdef USE_PTHREADS
#include <pthread.h>

#define NUM_READS 2

pthread_attr_t readAttr;
pthread_t readThread;

queue<int> readqueue;

pthread_mutex_t readQ;
pthread_cond_t readQWait;
int readwaiting;

pthread_mutex_t readDone[NUM_READS];
pthread_cond_t readDoneWait[NUM_READS];
int readcompleted[NUM_READS];

void* readin[NUM_READS];
void* readout[NUM_READS];
int readtype[NUM_READS];
size_t readelements[NUM_READS];

void* readAsync(void*)
{
  while(1)
    {
    // check the queue
    pthread_mutex_lock(&readQ);
    // if there's nothing, sleep
    if(readqueue.size() <= 0)
      {
      readwaiting = 1;
      pthread_cond_wait(&readQWait, &readQ);
      }
    readwaiting = 0;

    int which = readqueue.front();
    readqueue.pop();
    int type = readtype[which];
    size_t size = readelements[which];
    void* vin = readin[which];
    void* vout = readout[which];

    // done with the queue
    pthread_mutex_unlock(&readQ);

    // lock the readdone
    pthread_mutex_lock(&readDone[which]);

    // if it's 0, exit
    if(!vin)
      {
      pthread_mutex_unlock(&readDone[which]);
      break;
      }

    // do the read
    switch(type)
      {
      case 0:
        {
        point* in = (point*)vin;
        point* out = (point*)vout;
        for(size_t i = 0; i < size; i = i + 1)
          {
          out[i] = in[i];
          }
        }
        break;

      case 1:
        {
        float* in = (float*)vin;
        float* out = (float*)vout;
        for(size_t i = 0; i < size; i = i + 1)
          {
          out[i] = in[i];
          }
        }
        break;

      default:
        break;
      }

    // someone is waiting
    if(readcompleted[which] < 0)
      {
      pthread_cond_signal(&readDoneWait[which]);
      }
    readcompleted[which] = 1;

    // all done
    pthread_mutex_unlock(&readDone[which]);
    }

  pthread_exit(0);
}

void readStart(int which, void* in, void* out, int type, size_t elements)
{
  pthread_mutex_lock(&readQ);

  readqueue.push(which);
  readin[which] = in;
  readout[which] = out;
  readtype[which] = type;
  readelements[which] = elements;
  readcompleted[which] = 0;

  // someone is waiting to wake
  if(readwaiting)
    {
    pthread_cond_signal(&readQWait);
    }
  pthread_mutex_unlock(&readQ);
}

void readComplete(int which)
{
 pthread_mutex_lock(&readDone[which]);

 if(!readcompleted[which])
   {
   readcompleted[which] = -1;
   pthread_cond_wait(&readDoneWait[which], &readDone[which]);
   }

 pthread_mutex_unlock(&readDone[which]);
}

#endif
*/

int compff(const void* a, const void* b)
{
  floatfile* p = (floatfile*)a;
  floatfile* q = (floatfile*)b;

  if(p->v < q->v)
    {
    return -1;
    }
  
  if(p->v > q->v)
    {
    return 1;
    }

  return 0;
}

size_t partition(void* array, size_t len,
                 float pivot, int dim, int first)
{
  size_t left = 0;

  if(first)
    {
    point* p = (point*)array;

    switch(dim)
      {
      case 0:
        {
        for(size_t i = 0; i < len; i = i + 1)
          {    
          if(p[i].x < pivot)
            {
            point swap = p[left];
            p[left] = p[i];
            p[i] = swap;
            
            left = left + 1;
            }
          }
        }
        break;
        
      case 1:
        {
        for(size_t i = 0; i < len; i = i + 1)
          {    
          if(p[i].y < pivot)
            {
            point swap = p[left];
            p[left] = p[i];
            p[i] = swap;
            
            left = left + 1;
            }
          }
        }
        break;
        
      case 2:
        {
        for(size_t i = 0; i < len; i = i + 1)
          {    
          if(p[i].z < pivot)
            {
            point swap = p[left];
            p[left] = p[i];
            p[i] = swap;
            
            left = left + 1;
            }
          }
        }
        break;
        
      default:
        {
        }
        break;
      }
    }
  else
    {
    float* v = (float*)array;

    for(size_t i = 0; i < len; i = i + 1)
      {    
      if(v[i] < pivot)
        {
        float swap = v[left];
        v[left] = v[i];
        v[i] = swap;

        left = left + 1;
        }
      }
    }

  return left;
}

/*
// median of 5
size_t median5(void* array, int dim, int first)
{
  if(first)
    {
    point* p = (point*)array;

    switch(dim)
      {
      case 0:
        {
        if(p[0].x > p[1].x)
          {
          point temp = p[0];
          p[0] = p[1];
          p[1] = temp;
          }
        
        if(p[3].x > p[4].x)
          {
          point temp = p[3];
          p[3] = p[4];
          p[4] = temp;
          }
        
        if(p[0].x > p[3].x)
          {
          point temp = p[0];
          p[0] = p[3];
          p[3] = temp;
          }
        
        if(p[2].x > p[1].x)
          {
          if(p[1].x < p[3].x)
            {
            if(p[2].x < p[3].x)
              {
              return 2;
              }
            else
              {
              return 3;
              }
            }
          else
            {
            if(p[1].x < p[4].x)
              {
              return 1;
              }
            else
              {
              return 4;
              }
            }
          }
        else
          {
          if(p[2].x > p[3].x)
            {
            if(p[2].x < p[4].x)
              {
              return 2;
              }
            else
              {
              return 4;
              }
            }
          else
            {
            if(p[1].x < p[3].x)
              {
              return 1;
              }
            else
              {
              return 3;
              }
            }
          }
        }
        break;

      case 1:
        {
        if(p[0].y > p[1].y)
          {
          point temp = p[0];
          p[0] = p[1];
          p[1] = temp;
          }
        
        if(p[3].y > p[4].y)
          {
          point temp = p[3];
          p[3] = p[4];
          p[4] = temp;
          }
        
        if(p[0].y > p[3].y)
          {
          point temp = p[0];
          p[0] = p[3];
          p[3] = temp;
          }
        
        if(p[2].y > p[1].y)
          {
          if(p[1].y < p[3].y)
            {
            if(p[2].y < p[3].y)
              {
              return 2;
              }
            else
              {
              return 3;
              }
            }
          else
            {
            if(p[1].y < p[4].y)
              {
              return 1;
              }
            else
              {
              return 4;
              }
            }
          }
        else
          {
          if(p[2].y > p[3].y)
            {
            if(p[2].y < p[4].y)
              {
              return 2;
              }
            else
              {
              return 4;
              }
            }
          else
            {
            if(p[1].y < p[3].y)
              {
              return 1;
              }
            else
              {
              return 3;
              }
            }
          }
        }
        break;

      case 2:
        {
        if(p[0].z > p[1].z)
          {
          point temp = p[0];
          p[0] = p[1];
          p[1] = temp;
          }
        
        if(p[3].z > p[4].z)
          {
          point temp = p[3];
          p[3] = p[4];
          p[4] = temp;
          }
        
        if(p[0].z > p[3].z)
          {
          point temp = p[0];
          p[0] = p[3];
          p[3] = temp;
          }
        
        if(p[2].z > p[1].z)
          {
          if(p[1].z < p[3].z)
            {
            if(p[2].z < p[3].z)
              {
              return 2;
              }
            else
              {
              return 3;
              }
            }
          else
            {
            if(p[1].z < p[4].z)
              {
              return 1;
              }
            else
              {
              return 4;
              }
            }
          }
        else
          {
          if(p[2].z > p[3].z)
            {
            if(p[2].z < p[4].z)
              {
              return 2;
              }
            else
              {
              return 4;
              }
            }
          else
            {
            if(p[1].z < p[3].z)
              {
              return 1;
              }
            else
              {
              return 3;
              }
            }
          }
        }
        break;

      }
    }
  else
    {
    }
}

// median of medians
size_t medianofmedians(void* array, size_t len,
                       int dim, int first)
{
  if(first)
    {
    point* p = (point*)array;
    size_t trunc = len - (len % 5);
    size_t mlen = 0;

    for(size_t i = 0; i < trunc; i = i + 5)
      {
      size_t m = median5((void*)&(p[i]), dim, first);

      point temp = p[i + m];
      p[i + m] = p[mlen];
      p[mlen] = temp;
      
      mlen = mlen + 1;
      }

    return mlen;
    }
  else
    {
    float* v = (float*)array;
    size_t trunc = len - (len % 5);
    size_t mlen = 0;

    for(size_t i = 0; i < trunc; i = i + 5)
      {
      size_t m = median5((void*)&(v[i]), dim, first);

      float temp = v[i + m];
      v[i + m] = v[mlen];
      v[mlen] = temp;
      
      mlen = mlen + 1;
      }

    return mlen;
    }
}
*/

// quickselect
void select(void* array, size_t len, size_t k, 
            int dim, int first)
{
  size_t lastk = 0;
  size_t lastleft = 0;
  int samecount = 0;

  while(1) 
    {
    float pivot;

    if(first)
      {
      /*
      point* p = (point*)array;

      size_t mlen = medianofmedians(p, len, dim, first);
      while(mlen > 0)
        {
        mlen = medianofmedians(p, mlen, dim, first);
        }
      
      switch(dim)
        {
        case 0:
          {
          pivot = p[0].x;
          }
          break;

        case 1:
          {
          pivot = p[0].y;
          }
          break;

        case 2:
          {
          pivot = p[0].z;
          }
          break;

        default:
          {
          pivot = 0.0f;
          }
        }
      */

      point* p = (point*)array;
      size_t pi = (size_t)((double)rand() / RAND_MAX * len);
      pi = pi > len - 1 ? len - 1 : pi;
      
      switch(dim)
        {
        case 0:
          {
          pivot = p[pi].x;
          }
          break;
          
        case 1:
          {
          pivot = p[pi].y;
          }
          break;
          
        case 2:
          {
          pivot = p[pi].z;
          }
          break;
          
        default:
          {
          pivot = 0.0f;
          }
          break;
        }
      }
    else
      {
      float* v = (float*)array;
      size_t pi = (size_t)((double)rand() / RAND_MAX * len);
      pi = pi > len - 1 ? len - 1 : pi;

      pivot = v[pi];
      }

    // partition
    size_t left = partition(array, len, pivot, dim, first);
    printf("left: %lu k: %lu\n", (unsigned long int)left, (unsigned long int)k);

    // repeated too many times
    if(lastk == k && lastleft == left)
      {
      samecount = samecount + 1;
      }
    else
      {
      samecount = 0;
      }

    lastk = k;
    lastleft = left;

    // we found it
    if(k == left || samecount > 3)
      {
      break;
      }
    
    // select on the larger
    if(k < left)
      {
      // array = array
      // k = k
      len = left;
      }
    else
      {
      if(first)
        {
        array = &(((point*)array)[left]);
        }
      else
        {
        array = &(((float*)array)[left]);
        }
      k = k - left;
      len = len - left;
      }
    }
}

void permutation(point* map, size_t len,
                 FILE* meta, char* base, int maxdepth, int piece)
{
  // open the file for the random points
  char* fn = new char[255 + strlen(base)];
  sprintf(fn, "%s-%d-%d", base, maxdepth, piece);
  FILE* out = fopen(fn, "w");
  printf("writing %s\n", fn);
  delete [] fn;
  
  // make a permutation
  point perm[len];  // this assumes you can read in an entire block

  for(size_t i = 0; i < len; i = i + 1)
    {
    perm[i] = map[i];
    }
  
  // find the bounds
  float bounds[6] = {FLT_MAX, FLT_MIN,
                     FLT_MAX, FLT_MIN,
                     FLT_MAX, FLT_MIN};

  // get some guarantees
  float v[6] = {FLT_MAX, FLT_MIN,
                FLT_MAX, FLT_MIN,
                FLT_MAX, FLT_MIN};
  size_t vi[6] = {0, 0, 0, 0, 0, 0};

  for(size_t i = 0; i < len; i = i + 1)
    {
      bounds[0] = bounds[0] < perm[i].x ? bounds[0] : perm[i].x;
      bounds[1] = bounds[1] > perm[i].x ? bounds[1] : perm[i].x;
      bounds[2] = bounds[2] < perm[i].y ? bounds[2] : perm[i].y;
      bounds[3] = bounds[3] > perm[i].y ? bounds[3] : perm[i].y;
      bounds[4] = bounds[4] < perm[i].z ? bounds[4] : perm[i].z;
      bounds[5] = bounds[5] > perm[i].z ? bounds[5] : perm[i].z;

      if(v[0] > perm[i].vx)
        {
        v[0] = perm[i].vx;
        vi[0] = i;
        }

      if(v[1] < perm[i].vx)
        {
        v[1] = perm[i].vx;
        vi[1] = i;
        }

      if(v[2] > perm[i].vy)
        {
        v[2] = perm[i].vy;
        vi[2] = i;
        }

      if(v[3] < perm[i].vy)
        {
        v[3] = perm[i].vy;
        vi[3] = i;
        }

      if(v[4] > perm[i].vz)
        {
        v[4] = perm[i].vz;
        vi[4] = i;
        }

      if(v[5] < perm[i].vz)
        {
        v[5] = perm[i].vz;
        vi[5] = i;
        }
    }

  // move the guaranteed points into the sample
  for(size_t i = 0; i < 5; i = i + 1)
    {
    point temp = perm[vi[i]];
    perm[vi[i]] = perm[i];
    perm[i] = temp;
    }

  // scramble
  for(size_t i = 6; i < len; i = i + 1)
    {
      size_t swap = 
        (size_t)((double)rand() / RAND_MAX * (len - 6) + 6);
      swap = swap > len - 1 ? len - 1 : swap;

      point temp = perm[swap];
      perm[swap] = perm[i];
      perm[i] = temp;
    }

  // write it back out
  fwrite(perm, sizeof(point), len, out);

  // write the meta data
  fprintf(meta, "%d %d %f %f %f %f %f %f\n", 
          maxdepth, piece, 
          bounds[0], bounds[1], bounds[2], bounds[3], bounds[4], bounds[5]);

  // this is slow fwiw
  // fseek sucks
  /*
  size_t perm[numpoints];
  for(size_t i = 0; i < numpoints; i = i + 1)
    {
      perm[i] = i;
    }
  
  for(size_t i = 0; i < numpoints; i = i + 1)
    {
      size_t swap = 
        (size_t)((double)rand() / RAND_MAX * numpoints);
      swap = swap > numpoints - 1 ? numpoints - 1 : swap;
      
      size_t temp = perm[swap];
      perm[swap] = perm[i];
      perm[i] = temp;
    }
  
  // write the permuted file
  for(size_t i = 0; i < numpoints; i = i + 1)
    {
      point p;
      fseek(part, sizeof(point) * perm[i], SEEK_SET);
      fread(&p, sizeof(point), 1, part);
      fwrite(&p, sizeof(point), 1, out);
    }
  */

  fclose(out);
}

int writeSplits2(point* map, size_t len,
                   char* base, int depth, int dim,
                   FILE* meta, int maxdepth, int piece)
{
  printf("%d depth %d dim %d piece #\n", depth, dim, piece);

  // base case
  if(depth < 1)
    {
    permutation(map, len, meta, base, maxdepth, piece);
    return piece + 1;
    }    

  // find the pivots
  select(map, len, len / 2, dim, 1);

  // partitioned already

  // recurse
  dim = dim + 1;
  depth = dim > 2 ? depth - 1 : depth;
  dim = dim % 3;

  piece = writeSplits2(map, len / 2, base, depth, 
                       dim, meta, maxdepth, piece);
  piece = writeSplits2(&(map[len / 2]), len - len / 2, base, depth,
                       dim, meta, maxdepth, piece);

  return piece;

}

void merge(FILE* meta, char* base, int maxdepth, int splits)
{
  int depth = maxdepth - 1;
  int totalpieces = (int)pow(splits, maxdepth);

  while(depth >= 0)
    {
      // figure out how many pieces to merge
      int mergepieces = (int)pow(splits, maxdepth - depth);
      int outcount = 0;

      for(int i = 0; i < totalpieces; i = i + mergepieces)
        {
          // output file
          char* fn = new char[255 + strlen(base)];
          sprintf(fn, "%s-%d-%d", base, depth, outcount);
          FILE* out = fopen(fn, "w");
          printf("merging %s\n", fn);

          float bounds[6] = {FLT_MAX, FLT_MIN,
                             FLT_MAX, FLT_MIN,
                             FLT_MAX, FLT_MIN};
          
          // open and copy points
          for(int j = i; j < i + mergepieces; j = j + 1)
            {
              sprintf(fn, "%s-%d-%d", base, maxdepth, j);
              FILE* in = fopen(fn, "r");
              
              fseek(in, 0, SEEK_END);
              size_t filetotal = ftell(in) / sizeof(point);
              rewind(in);
              
              // take a fraction of in to out
              filetotal = filetotal / mergepieces;
              point perm[filetotal];
              fread(perm, sizeof(point), filetotal, in);
              fwrite(perm, sizeof(point), filetotal, out);
              fclose(in);

              // get bounds
              for(size_t k = 0; k < filetotal; k = k + 1)
                {
                  bounds[0] = bounds[0] < perm[k].x ? bounds[0] : perm[k].x;
                  bounds[1] = bounds[1] > perm[k].x ? bounds[1] : perm[k].x;
                  bounds[2] = bounds[2] < perm[k].y ? bounds[2] : perm[k].y;
                  bounds[3] = bounds[3] > perm[k].y ? bounds[3] : perm[k].y;
                  bounds[4] = bounds[4] < perm[k].z ? bounds[4] : perm[k].z;
                  bounds[5] = bounds[5] > perm[k].z ? bounds[5] : perm[k].z;
                }
            }

          // write bounds
          // write the meta data
          fprintf(meta, "%d %d %f %f %f %f %f %f\n", 
                  depth, outcount, 
                  bounds[0], bounds[1], 
                  bounds[2], bounds[3], 
                  bounds[4], bounds[5]);

          // increment to next piece
          outcount = outcount + 1;
          fclose(out);
          delete [] fn;
        }

      // go to next depth
      depth = depth - 1;
    }
} 

int main(int argc, char* argv[])
{
  if(argc != 3) 
    {
    fprintf(stderr, "%s <file> <level>\n", argv[0]);
    exit(0);
    }
  
  FILE* fp = fopen(argv[1], "a+");
  if (!fp)
    {
    fprintf(stderr, "File [%s] not found.\n", argv[1]);
    exit(0);
    }

#ifdef __APPLE__
  mach_timebase_info_data_t timebase;
  mach_timebase_info(&timebase);
  uint64_t start = mach_absolute_time();
#endif

/*
#ifdef USE_PTHREADS
  pthread_mutex_init(&readQ, 0);
  pthread_cond_init(&readQWait, 0);
  readwaiting = 0;

  for(int i = 0; i < NUM_READS; i = i + 1)
    {
    readcompleted[i] = 0;
    pthread_mutex_init(&readDone[i], 0);
    pthread_cond_init(&readDoneWait[i], 0);
    }

  pthread_attr_init(&readAttr);
  pthread_attr_setdetachstate(&readAttr, PTHREAD_CREATE_DETACHED);

  pthread_create(&readThread, &readAttr, readAsync, 0);

  pthread_attr_destroy(&readAttr);
#endif
*/

  int level = atoi(argv[2]);

  char* fn = new char[255 + strlen(argv[1])];
  sprintf(fn, "%s.meta", argv[1]);
  FILE* meta = fopen(fn, "w");
  delete [] fn;

  // kd sort the points
  fseek(fp, 0, SEEK_END);
  size_t memlen = ftell(fp);
  size_t len = memlen / sizeof(point);

  size_t pagesize = getpagesize();
  memlen = memlen + (memlen % pagesize > 0 ?
                     pagesize - memlen % pagesize : 0);
  point* map = (point*)
    mmap(0, memlen, PROT_READ | PROT_WRITE, MAP_FILE | MAP_SHARED, fileno(fp), 0);
  
  writeSplits2(map, len, argv[1], level, 0, meta, level, 0);

  munmap(map, memlen);

  fclose(fp);

  // merge low levels into high
  merge(meta, argv[1], level, 8);
  fclose(meta);

#ifdef __APPLE__
  uint64_t end = mach_absolute_time();
  size_t elapsed = (end - start) * timebase.numer / timebase.denom;
  printf("overall: %f s\n", elapsed / 1000000000.0);
#endif

/*
#ifdef USE_PTHREADS
  readStart(0, 0, 0, 0, 0);
  
  
  for(int i = 0; i < NUM_READS; i = i + 1)
    {
    pthread_mutex_destroy(&readDone[i]);
    pthread_cond_destroy(&readDoneWait[i]);
    }
  pthread_mutex_destroy(&readQ);
  pthread_cond_destroy(&readQWait);
  

  pthread_exit(0);
#endif
*/
}
