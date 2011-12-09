#include <stdio.h>
#include <math.h>
#include <string.h>


/*
  g++ -o quad2tri quad2tri.C
*/


static void WriteBC( int i, int j, FILE *fp,  FILE *out);

main(int argc, char *argv[]){
  register int i,j;
  int n;
  double x[4],y[4];
  FILE *fp, *out = stdout;
  char buf[BUFSIZ];


  if(argc != 2){
    fprintf(stderr,"Usage: quad2tri file.rea\n");
    exit(1);
  }

  if(!(fp = fopen(argv[argc-1],"r"))){
    fprintf(stderr,"File %s not found\n",argv[argc-1]);
    exit(1);
  }

  fgets(buf,BUFSIZ,fp);  fputs(buf,out);
  fgets(buf,BUFSIZ,fp);  fputs(buf,out);
  fgets(buf,BUFSIZ,fp);  fputs(buf,out);
  fgets(buf,BUFSIZ,fp);  fputs(buf,out);
  sscanf(buf,"%d",&n);
  // read and write parameter list
  for(i = 0; i < n; ++i){
    fgets(buf,BUFSIZ,fp);
    fputs(buf,out);
  }

  // read and write upto start of mesh
  for(i = 0; i < 4; ++i){
    fgets(buf,BUFSIZ,fp);
    fputs(buf,out);
  }

  fgets(buf,BUFSIZ,fp);
  sscanf(buf,"%d",&n);
  fprintf(out," %d %d     NEL, NDIM\n",2*n,2);
  for(i = 0; i < n; ++i){
    fgets(buf,BUFSIZ,fp);
    fscanf(fp,"%lf%lf%lf%lf\n",x,x+1,x+2,x+3);
    fscanf(fp,"%lf%lf%lf%lf\n",y,y+1,y+2,y+3);
    fprintf(out,"\t Element   %d\n",2*i+1);
    fprintf(out,"%lf %lf %lf %lf\n",x[0],x[1],x[2],x[0]);
    fprintf(out,"%lf %lf %lf %lf\n",y[0],y[1],y[2],y[0]);
    fprintf(out,"\t Element   %d\n",2*i+2);
    fprintf(out,"%lf %lf %lf %lf\n",x[2],x[3],x[0],x[2]);
    fprintf(out,"%lf %lf %lf %lf\n",y[2],y[3],y[0],y[2]);
  }

  // read past curved data at present
  for(i = 0; i < 4; ++i){
    fgets(buf,BUFSIZ,fp);
    fputs(buf,out);
  }

  for(i = 0; i < n; ++i){
    for(j = 0; j < 2; ++j)
      WriteBC(2*i,j,fp,out);

    fprintf(out,"E %d %d %d %d\n",2*i+1,3,2*i+2,3);
    fprintf(out,"Dummy line\n");

    for(j = 0; j < 2; ++j)
      WriteBC(2*i+1,j,fp,out);

    fprintf(out,"E %d %d %d %d\n",2*i+2,3,2*i+1,3);
    fprintf(out,"Dummy line\n");
  }

  while(fgets(buf,BUFSIZ,fp))
    fputs(buf,out);

}

static void WriteBC(int i, int j, FILE *fp, FILE *out){
  double eid,side;
  double v[2];
  char c, buf[BUFSIZ];

  fgets(buf,BUFSIZ,fp);
  sscanf(buf,"%1s",&c);

  switch(c){
  case 'W':
    fprintf(out,"W %d %d \n",i+1,j+1);
    break;
  case 'E': case 'P':
    sscanf(buf,"%*1s%*d%*d%lf%lf",&eid,&side);
    if(side < 3)
      fprintf(out,"%c %d %d %d %d\n",c,i+1,j+1,(int) (2*eid-1),(int) side);
    else
      fprintf(out,"%c %d %d %d %d\n",c,i+1,j+1,(int) (2*eid),(int) (side-2));
    break;
  case 'V': case 'O': case 'F':
    sscanf(buf,"%*1s%d%d%lf%lf",v,v+1);
    fprintf(out,"%c %d %d %d %d\n",c,i+1,j+1,v[0],v[1]);
    break;
  case 'v': case 'f':
    fprintf(out,"%c %d %d\n",c,i+1,j+1);
    fgets(buf,BUFSIZ,fp);  fputs(buf,out);
    fgets(buf,BUFSIZ,fp);  fputs(buf,out);
    break;
  }
}
