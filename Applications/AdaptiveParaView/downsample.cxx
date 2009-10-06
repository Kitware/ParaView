#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef __APPLE__
#include <mach/mach_time.h>
#endif


// calculate sampling levels and blocks
inline void sampleRates(size_t* r, size_t* s, size_t* t,
      size_t* u, size_t* v, size_t* w,
      size_t x, size_t y, size_t z,
      size_t height, size_t degree, size_t rate)
{
  r[0] = 1;
  s[0] = 1;
  t[0] = 1;

  u[0] = x;
  v[0] = y;
  w[0] = z;

  size_t level = 1;

  while(level < height) 
    {
    r[level] = r[level - 1];
    s[level] = s[level - 1];
    t[level] = t[level - 1];

    for(int d = 0; d < degree; d = d + 1) 
      {
      if(z >= y && z >= x) 
        {
        t[level] = t[level] * rate;
        z = z / rate + (z % rate > 0 ? 1 : 0);
        }
      else if(y >= x) 
        {
        s[level] = s[level] * rate;
        y = y / rate + (y % rate > 0 ? 1 : 0);
        }
      else 
        {
        r[level] = r[level] * rate;
        x = x / rate + (x % rate > 0 ? 1 : 0);
        }
      }
    
    u[level] = x;
    v[level] = y;
    w[level] = z;
    
    level = level + 1;
    }
}

int main(int argc, char* argv[]) {
  if(argc != 8) 
    {
    fprintf(stderr, "%s <file> <height> <degree> <rate> <i_#pts> <j_#pts> <k_#pts>\n", argv[0]);
    exit(0);
    }
  
  FILE* fp = fopen(argv[1], "r");
  if (!fp)
    {
    fprintf(stderr, "File [%s] not found.\n", argv[1]);
    exit(0);
    }
  
  unsigned int height = atol(argv[2])+1;
  unsigned int degree = atol(argv[3]);
  unsigned int rate = atol(argv[4]);
  
  size_t x = atol(argv[5]);
  size_t y = atol(argv[6]);
  size_t z = atol(argv[7]);
  
  size_t r[height];  // sample rate at a level
  size_t s[height];
  size_t t[height];
  
  size_t u[height];  // dimensions at level
  size_t v[height];
  size_t w[height];
  
  sampleRates(r, s, t, u, v, w, x, y, z, height, degree, rate);
  
  fseek(fp, 0, SEEK_END);
  size_t numfloats = ftell(fp) / sizeof(float);
  fseek(fp, 0, SEEK_SET);
  if(x * y * z != numfloats) 
    {
    fprintf(stderr, "dimensions are not the same as the file size\n");
    exit(0);
    }
  
  FILE** output = new FILE*[height];
  for(unsigned int h = 1; h < height; h = h + 1) 
    {
    char fn[strlen(argv[1]) + 256];
    sprintf(fn, "%s-%d", argv[1], h);
    output[h] = fopen(fn, "w");
    
    printf("%d %d %d = 1 / %d = %d\n", 
           u[h], v[h], w[h],
           r[h] * s[h] * t[h], 
           u[h] * v[h] * w[h] * sizeof(float));
    }
  
#ifdef __APPLE__
  mach_timebase_info_data_t timebase;
  mach_timebase_info(&timebase);
  uint64_t start = mach_absolute_time();
#endif

  for(size_t k = 0; k < z; k = k + 1) 
    {
    for(size_t j = 0; j < y; j = j + 1) 
      {
      for(size_t i = 0; i < x; i = i + 1) 
        {
        float value;
        fread(&value, sizeof(float), 1, fp);
        
        for(unsigned int h = 1; h < height; h = h + 1) 
          {
          if((i % r[h] == 0) && (j % s[h] == 0) && (k % t[h] == 0)) 
            {
            fwrite(&value, sizeof(float), 1, output[h]);
            }
          }
        }
      }
    }

  fclose(fp);
  for(unsigned int h = 1; h < height; h = h + 1) 
    {
    fclose(output[h]);
    }
  delete [] output;

#ifdef __APPLE__
  uint64_t end = mach_absolute_time();
  uint64_t elapsed = (end - start) * timebase.numer / timebase.denom;
  printf("overall: %f s\n", elapsed / 1000000000.0);
#endif

}
