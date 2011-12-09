#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*-------------------------------------------------------------------------*
 * This is a routine to add +xvalue to the x coordinate and +yvalue        *
 * to the y coordinate of an rea file.                                     *
 * Usage:    mvgrid +xvalue +yvalue +zvalue file[.rea]                     *
 *-------------------------------------------------------------------------*/
/* only needed to compile with gen_utils.o */
char *prog   = "datmv";
char *usage  = "datmv\n";
char *author = "";
char *rcsid  = "";
char *help   = "";

main(int argc, char *argv[])
{
  register int i,j;
  char file[BUFSIZ],buf[BUFSIZ];
  int n1,n2,old=0,ncurv,nsurf;
  FILE *fp,*fp_new;

  if(argc != 2){
    fprintf(stdout,"Usage:    dat2tec file[.dat] \n");
    exit(-1);
  }

  if(!strstr(argv[argc-1],".dat"))
    sprintf(file,"%s.dat",argv[argc-1]);
  else
    sprintf(file,"%s",argv[argc-1]);

  if(!(fp = fopen(file,"r"))){
    fprintf(stdout,"File %s does not exist\n",file);
    exit(-1);
  }

  fp_new = stdout;

  fprintf(fp_new,"VARIABLES = x y z\n");

  fgets(buf,BUFSIZ,fp);
  if(strstr(buf,"Old")) old = 1;
  fgets(buf,BUFSIZ,fp);
  sscanf(buf,"%d%d\n",&ncurv,&nsurf);
  fgets(buf,BUFSIZ,fp);

  for(i = 0; i < ncurv; ++i){
    fgets(buf,BUFSIZ,fp);
    if(old)
      sscanf(buf,"%*d%*d%d\n",&n1);
    else{
      fgets(buf,BUFSIZ,fp);
      sscanf(buf,"%d\n",&n1);
    }

    for(j = 0; j < n1; ++j){
      fgets(buf,BUFSIZ,fp);
    }
  }

  fgets(buf,BUFSIZ,fp);

  for(i = 0; i < nsurf; ++i){
    fgets(buf,BUFSIZ,fp);
    if(old)
      sscanf(buf,"%*d%*d%d%d\n",&n1,&n2);
    else{
      fgets(buf,BUFSIZ,fp);
      sscanf(buf,"%d%d\n",&n1,&n2);
    }

    fprintf(fp_new,"Zone T=\"Surface %d \" I=%d, J=%d, F=POINT\n",i+1,n1,n2);
    for(j = 0; j < n1*n2; ++j){
      fgets(buf,BUFSIZ,fp);
      fputs(buf,fp_new);
    }
  }
  fclose(fp);
  fclose(fp_new);

  return 0;
}
