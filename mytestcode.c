#include "fcntl.h"
#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"

//Function taken from usertests.c and slightly modified.
void
opentest(void)
{
  int fd;

  fd = open("/mnt/temporary.txt", O_RDONLY);
  if(fd < 0){
    printf(1, "open /mnt/temporary.txt failed!\n");
  }
  close(fd);
  fd = open("/mnt/hello.txt", O_RDONLY);
  if(fd >= 0){
    printf(1, "open /mnt/hello.txt succeeded!\n");
  }
  printf(1, "open test ok\n");
}

int main(){
  int fd, myret;
  char ch[201], ch2[46], ch3[53], w1[81], w2[81];
  ch[200] = '\0';
  ch2[45] = '\0';
  ch3[52] = '\0';
  
  //Testing open functionality
  printf(1, "\nOPEN TESTS START\n");
  opentest();

  //Testing read functionality
  printf(1, "\nREAD TESTS START\n");

  //Case 1
  printf(1, "\nCase 1 : Reading a small file in root directory\n");
  fd = open("/mnt/hello.txt", O_RDONLY);
  myret = read(fd, ch2, 45);
  //printf(1, "%s", ch2);
  if(myret == 45){
    printf(1, "\nTest case passed\n");
  }
  else{
    printf(1, "\nTest case failed\n");
  }
  close(fd);

  //Case 2
  printf(1, "\nCase 2 : Reading a file inside a directory\n");
  fd = open("/mnt/new/hello.txt", O_RDONLY);
  myret = read(fd, ch3, 52);
  //printf(1, "%s", ch3);
  if(myret == 52){
    printf(1, "\nTest case passed\n");
  }
  else{
    printf(1, "\nTest case failed\n");
  }
  close(fd);

  //Case 3
  printf(1, "\nCase 3 : Reading a file with large bytes\n");
  fd = open("/mnt/README", O_RDONLY);
  myret = read(fd, ch, 200);
  //printf(1, "%s", ch);
  if(myret == 200){
    printf(1, "\nTest case passed\n");
  }
  else{
    printf(1, "\nTest case failed\n");
  }
  close(fd);
  
  //Testing write functionality.
  printf(1, "\nWRITE TESTS START\n");

  //Case 1
  printf(1, "\nCase 1: writing to a small file\n");
  fd = open("/mnt/justtest.txt", O_WRONLY);
  for(int j=0; j<80; j++){
    w1[j] = 'c';
  }
  w1[80] = '\0';
  write(fd, w1, 80);
  close(fd);
  fd = open("/mnt/justtest.txt", O_RDONLY);
  read(fd, w2, 80);
  close(fd);
  int i1;
  for(i1=0; i1<80; i1++){
    if(w1[i1] != w2[i1])
      break;
  }
  if(i1 == 80){
    printf(1, "\nTest case passed\n");
  }
  else{
    printf(1, "\nTest case failed\n");
  }

  //Case 2
  printf(1, "\nCase 2: writing to a file inside a directory\n");
  fd = open("/mnt/new/fortest.txt", O_WRONLY);
  for(int k=0; k<80; k++){
    w1[k] = 'p';
  }
  w1[80] = '\0';
  write(fd, w1, 80);
  close(fd);
  fd = open("/mnt/new/fortest.txt", O_RDONLY);
  read(fd, w2, 80);
  close(fd);
  for(i1=0; i1<80; i1++){
    if(w1[i1] != w2[i1])
      break;
  }
  if(i1 == 80){
    printf(1, "\nTest case passed\n");
  }
  else{
    printf(1, "\nTest case failed\n");
  }

  //Testing cat commnd
  printf(1, "\nCAT COMMAND TESTS START\n");
  int pid1, pid2;
  char *catarr1[] = {"cat", "/mnt/new/new1/testnew.txt", 0};
  char *catarr2[] = {"cat", "/mnt/kill.c", 0};
  
  //cat on file in root directory
  printf(1, "\nCase 6: cat on a file in root directory\n");
  pid1 = fork();
  if(pid1 == 0){
    exec("cat", catarr2);
  }
  else{
    wait();
  }

  //cat on file inside folders.
  printf(1, "\nCase 7: cat on file in nested directory\n");
  pid2 = fork();
  if(pid2 == 0){
    exec("cat", catarr1);
  }
  else{
    wait();
  }
  exit();
}
