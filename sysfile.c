//
// File-system system calls.
// Mostly argument checking, since we don't trust
// user code, and calls into file.c and fs.c.
//

#include "types.h"
#include "defs.h"
#include "param.h"
#include "stat.h"
#include "mmu.h"
#include "proc.h"
#include "fs.h"
#include "file.h"
#include "fcntl.h"

// Fetch the nth word-sized system call argument as a file descriptor
// and return both the descriptor and the corresponding struct file.
static int
argfd(int n, int *pfd, struct file **pf)
{
  int fd;
  struct file *f;

  if(argint(n, &fd) < 0)
    return -1;
  if(fd < 0 || fd >= NOFILE || (f=proc->ofile[fd]) == 0)
    return -1;
  if(pfd)
    *pfd = fd;
  if(pf)
    *pf = f;
  return 0;
}

// Allocate a file descriptor for the given file.
// Takes over file reference from caller on success.
static int
fdalloc(struct file *f)
{
  int fd;

  for(fd = 0; fd < NOFILE; fd++){
    if(proc->ofile[fd] == 0){
      proc->ofile[fd] = f;
      return fd;
    }
  }
  return -1;
}

int
sys_dup(void)
{
  struct file *f;
  int fd;

  if(argfd(0, 0, &f) < 0)
    return -1;
  if((fd=fdalloc(f)) < 0)
    return -1;
  filedup(f);
  return fd;
}

int
sys_read(void)
{
  struct file *f;
  int n;
  char *p;

  if(argfd(0, 0, &f) < 0 || argint(2, &n) < 0 || argptr(1, &p, n) < 0)
    return -1;
  return fileread(f, p, n);
}

int
sys_write(void)
{
  struct file *f;
  int n;
  char *p;

  if(argfd(0, 0, &f) < 0 || argint(2, &n) < 0 || argptr(1, &p, n) < 0)
    return -1;
  return filewrite(f, p, n);
}

int
sys_close(void)
{
  int fd;
  struct file *f;

  if(argfd(0, &fd, &f) < 0)
    return -1;
  proc->ofile[fd] = 0;
  fileclose(f);
  return 0;
}

int
sys_fstat(void)
{
  struct file *f;
  struct stat *st;

  if(argfd(0, 0, &f) < 0 || argptr(1, (void*)&st, sizeof(*st)) < 0)
    return -1;
  return filestat(f, st);
}

// Create the path new as a link to the same inode as old.
int
sys_link(void)
{
  char name[DIRSIZ], *new, *old;
  struct inode *dp, *ip;

  if(argstr(0, &old) < 0 || argstr(1, &new) < 0)
    return -1;
  if((ip = namei(old)) == 0)
    return -1;

  begin_trans();

  ilock(ip);
  if(ip->type == T_DIR){
    iunlockput(ip);
    commit_trans();
    return -1;
  }

  ip->nlink++;
  iupdate(ip);
  iunlock(ip);

  if((dp = nameiparent(new, name)) == 0)
    goto bad;
  ilock(dp);
  if(dp->dev != ip->dev || dirlink(dp, name, ip->inum) < 0){
    iunlockput(dp);
    goto bad;
  }
  iunlockput(dp);
  iput(ip);

  commit_trans();

  return 0;

bad:
  ilock(ip);
  ip->nlink--;
  iupdate(ip);
  iunlockput(ip);
  commit_trans();
  return -1;
}

// Is the directory dp empty except for "." and ".." ?
static int
isdirempty(struct inode *dp)
{
  int off;
  struct dirent de;

  for(off=2*sizeof(de); off<dp->size; off+=sizeof(de)){
    if(readi(dp, (char*)&de, off, sizeof(de)) != sizeof(de))
      panic("isdirempty: readi");
    if(de.inum != 0)
      return 0;
  }
  return 1;
}

//PAGEBREAK!
int
sys_unlink(void)
{
  struct inode *ip, *dp;
  struct dirent de;
  char name[DIRSIZ], *path;
  uint off;

  if(argstr(0, &path) < 0)
    return -1;
  if((dp = nameiparent(path, name)) == 0)
    return -1;
  begin_trans();

  ilock(dp);

  // Cannot unlink "." or "..".
  if(namecmp(name, ".") == 0 || namecmp(name, "..") == 0)
    goto bad;

  if((ip = dirlookup(dp, name, &off)) == 0)
    goto bad;
  ilock(ip);

  if (ip->password[0] !=0 && proc->inode[ip->inum] == 0 ) { //if we have password and file has no premmision.
        iunlockput(ip);
        return -1;
    }
  if(ip->nlink < 1)
    panic("unlink: nlink < 1");
  if(ip->type == T_DIR && !isdirempty(ip)){
    iunlockput(ip);
    goto bad;
  }

  memset(&de, 0, sizeof(de));
  if(writei(dp, (char*)&de, off, sizeof(de)) != sizeof(de))
    panic("unlink: writei");
  if(ip->type == T_DIR){
    dp->nlink--;
    iupdate(dp);
  }
  iunlockput(dp);

  ip->nlink--;
  iupdate(ip);
  iunlockput(ip);

  commit_trans();

  return 0;

bad:
  iunlockput(dp);
  commit_trans();
  return -1;
}

//mode: 0 = dont handle symlinks, 1=follow symlinks
static struct inode*
create(char *path, short type, short major, short minor, int mode)
{
  uint off;
  struct inode *ip, *dp;
  char name[DIRSIZ];

  if((dp = nameiparent(path, name)) == 0)
    return 0;
  ilock(dp);

  if((ip = dirlookup(dp, name, &off)) != 0){
    iunlockput(dp);
    ilock(ip);

    if (mode && ip->type==T_SYMLINK)
      if ((ip=derefrenceSymlinkWrapper(ip,path))==0)
        return 0;

    if(type == T_FILE && ip->type == T_FILE)
      return ip;

    if (ip->password[0] !=0 && proc->inode[ip->inum] == 0 ) //if we have password and file has no premmision.
    {
        iunlockput(ip);
        return 0;
    }
    iunlockput(ip);
    return 0;
  }

  if((ip = ialloc(dp->dev, type)) == 0)
    panic("create: ialloc");

  ilock(ip);
  ip->major = major;
  ip->minor = minor;
  ip->nlink = 1;
  iupdate(ip);

  if(type == T_DIR){  // Create . and .. entries.
    dp->nlink++;  // for ".."
    iupdate(dp);
    // No ip->nlink++ for ".": avoid cyclic ref count.
    if(dirlink(ip, ".", ip->inum) < 0 || dirlink(ip, "..", dp->inum) < 0)
      panic("create dots");
  }

  if(dirlink(dp, name, ip->inum) < 0)
    panic("create: dirlink");

  iunlockput(dp);

  return ip;
}



static
int openWrapper(char *path, int omode, int mode){
    int fd;
    struct file *f;
    struct inode *ip;
    if (omode & O_CREATE)
    {
        begin_trans();
        ip = create(path, T_FILE, 0, 0, mode);
        commit_trans();
        if (ip == 0)
            return -1;
    }
    else
    {
        if ((ip = namei(path)) == 0)
            return -1;
        ilock(ip);

        if (mode && ip->type == T_SYMLINK)
            if ((ip = derefrenceSymlinkWrapper(ip, path)) == 0)
                return -1;

        if (ip->password[0] !=0 && proc->inode[ip->inum] == 0 ) //if we have password and file has no premmision.
        {
            iunlockput(ip);
            return -1;
        }
        if (ip->type == T_DIR && omode != O_RDONLY)
        {
            iunlockput(ip);
            return -1;
        }
    }
    if ((f = filealloc()) == 0 || (fd = fdalloc(f)) < 0)
    {
        if (f) fileclose(f);
        iunlockput(ip);
        return -1;
    }
    iunlock(ip);
    f->type = FD_INODE;
    f->ip = ip; f->off = 0;
    f->readable = !(omode & O_WRONLY);
    f->writable = (omode & O_WRONLY) || (omode & O_RDWR);
    return fd;
}

int
sys_openNoFollow(void)
{
  char *path;
  int omode;
  if(argstr(0, &path) < 0 || argint(1, &omode) < 0)
    return -1;
  return openWrapper(path,omode,0);
}

int
sys_open(void)
{
  char *path;
  int omode;
  if(argstr(0, &path) < 0 || argint(1, &omode) < 0)
    return -1;
  return openWrapper(path,omode,1);
}

int
sys_mkdir(void)
{
  char *path;
  struct inode *ip;

  begin_trans();
  if(argstr(0, &path) < 0 || (ip = create(path, T_DIR, 0, 0,0)) == 0){
    commit_trans();
    return -1;
  }
  iunlockput(ip);
  commit_trans();
  return 0;
}

int
sys_mknod(void)
{
  struct inode *ip;
  char *path;
  int len;
  int major, minor;

  begin_trans();
  if((len=argstr(0, &path)) < 0 ||
     argint(1, &major) < 0 ||
     argint(2, &minor) < 0 ||
     (ip = create(path, T_DEV, major, minor,0)) == 0){
    commit_trans();
    return -1;
  }
  iunlockput(ip);
  commit_trans();
  return 0;
}

int
sys_chdir(void)
{
  char *path;
  struct inode *ip;

  if(argstr(0, &path) < 0 || (ip = namei(path)) == 0)
    return -1;
  ilock(ip);
  if (ip->type == T_SYMLINK) 
    if ((ip=derefrenceSymlinkWrapper(ip,path))==0)
      return -1;

  if(ip->type != T_DIR){
    iunlockput(ip);
    return -1;
  }
  iunlock(ip);
  iput(proc->cwd);
  proc->cwd = ip;
  return 0;
}

int
sys_exec(void)
{
    char *path, *argv[MAXARG];
    int i;
    uint uargv, uarg;

    if (argstr(0, &path) < 0 || argint(1, (int *)&uargv) < 0) {
        return -1;
    }
    memset(argv, 0, sizeof(argv));
    for (i = 0;; i++) {
        if (i >= NELEM(argv))
            return -1;
        if (fetchint(proc, uargv + 4 * i, (int *)&uarg) < 0)
            return -1;
        if (uarg == 0) {
            argv[i] = 0;
            break;
        }
        if (fetchstr(proc, uarg, &argv[i]) < 0)
            return -1;
    }
    return exec(path, argv);
}

int
sys_pipe(void)
{
  int *fd;
  struct file *rf, *wf;
  int fd0, fd1;

  if(argptr(0, (void*)&fd, 2*sizeof(fd[0])) < 0)
    return -1;
  if(pipealloc(&rf, &wf) < 0)
    return -1;
  fd0 = -1;
  if((fd0 = fdalloc(rf)) < 0 || (fd1 = fdalloc(wf)) < 0){
    if(fd0 >= 0)
      proc->ofile[fd0] = 0;
    fileclose(rf);
    fileclose(wf);
    return -1;
  }
  fd[0] = fd0;
  fd[1] = fd1;
  return 0;
}


int
sys_symlink(void) {
  char* oldpath;
  char* newpath;
  struct inode *ip,*oldip;
  if(argstr(0, &oldpath) < 0)
    return -1;
  if(argstr(1, &newpath) < 0)
    return -1;

  int counter=1;
  oldip = namei(oldpath);
  if (oldip) {
  ilock(oldip);
  if (oldip->type==T_SYMLINK) {
    readi(oldip,(char*)&counter,0,4);
    counter++;
  }
  iunlockput(oldip);
  }
  if (counter>15)
    return -1;

  begin_trans();
  ip = create(newpath, T_SYMLINK, 0, 0 , 0);
  commit_trans();
  if(ip == 0) {
    return -1;
  }

  begin_trans();
  int flag=0;
  if ((writei(ip, (char*)&counter, 0, 4) < 4))
      flag =1;
  if ((writei(ip, oldpath, 4, strlen(oldpath))) < strlen(oldpath))
      flag =1;
  char* nil = "\0";
  if ((writei(ip, nil , 4+strlen(oldpath),1)) < 1)                  //null terminate string
      flag =1;
  iunlockput(ip);
  commit_trans();
  if (flag)
    return -1;
  return 0;
}

static int
readlink(char* pathname,char* buf, int bufsiz);
int
sys_readlink(void) {
  char* pathname;
  char* buf;
  int bufsiz;

  if(argstr(0, &pathname) < 0)
    return -2;
  if(argptr(1, &buf,1) < 0)
    return -2;
  if(argint(2 ,&bufsiz) < 0)
    return -2;
  return readlink(pathname,buf,bufsiz);

}

static int
findLastSlash(const char* buf) {
  int i;
  for (i=strlen(buf);i>=0;i--) {
    if (buf[i]=='/')
      return i+1;
  }
  return 0;
}

static int
readlink(char* pathname,char* buf, int bufsiz) {
  struct inode *ip;
  if((ip = namei(pathname)) == 0)
    return -1;
  ilock(ip);
  if (ip->type !=T_SYMLINK){
    iunlock(ip);
    return -1;
  }
  int linkjumpcounter=0;
  char temp=1;
  char nameTemp[14];

  char* mybuf = kalloc();
  memset(mybuf,0,4096);
  safestrcpy(mybuf,pathname,strlen(pathname)+1); //preper for line 584 in case symlink value is relative path
  char* mybuf2 = mybuf+2048;
  do {
    int i=0;
    while(temp && i < 2048) {
      if(readi(ip, &temp, 4+i, 1) != 1) {
        iunlockput(ip);
        kfree(mybuf);
        return -1;
      }
      mybuf2[i++]=temp;;
    }
    temp=1;
    iunlockput(ip);

    if (mybuf2[0]=='/')                                   //dealing with full path
      safestrcpy(mybuf,mybuf2,strlen(mybuf2)+1);
    else {                                                //relative path
        int last_slash=findLastSlash(mybuf);
        safestrcpy(mybuf+last_slash,mybuf2,strlen(mybuf2)+1);
      }

    ip=namex(mybuf,0,nameTemp,0);
    linkjumpcounter++;
    if (ip) ilock(ip); 
  } while (ip && ip->type==T_SYMLINK && linkjumpcounter<16);
  iunlockput(ip);
  
  int counter=strlen(mybuf);
  safestrcpy(buf,mybuf,bufsiz);
  kfree(mybuf);
  if (linkjumpcounter==16)
    return -1;
  return counter;
}


int
sys_fprot(void)
{
    char *pathname;
    char *password;
    struct inode *ip;
    int i;

    if (argstr(0, &pathname) < 0)
        return -2;
    if (argstr(1, &password) < 0)
        return -2;
    if ((ip = namei(pathname)) == 0)
        return -1;

    ilock(ip);
    if (ip->type != T_FILE) {
      iunlockput(ip);
      return -1;
    }

    if (ip->password[0] != 0 || ip->ref > 1 || strlen(password) > 10) {
        iunlockput(ip);
        return -1;
    }
    strncpy(ip->password, password, strlen(password));
    for (i = strlen(password); i < 10; i++)
        ip->password[i] = '\0';
    begin_trans();
    iupdate(ip);
    commit_trans();
    iunlockput(ip);
    return 0;
}

int
sys_funprot(void)
{
    char *pathname;
    char *password;
    struct inode *ip;
    if (argstr(0, &pathname) < 0)
        return -2;
    if (argstr(1, &password) < 0)
        return -2;
    if ((ip = namei(pathname)) == 0)
        return -1;
    ilock(ip);

    if (strcmp(ip->password, password) != 0 || ip->type != T_FILE) {
        iunlockput(ip);
        return -1;
    }
    ip->password[0] = 0;
    begin_trans();
    iupdate(ip);
    commit_trans();

    iunlockput(ip);
    return 0;

}

int
sys_funlock(void)
{
    char *pathname;
    char *password;
    struct inode *ip;
    if (argstr(0, &pathname) < 0)
        return -2;
    if (argstr(1, &password) < 0)
        return -2;
    if ((ip = namei(pathname)) == 0)
        return -1;
    ilock(ip);
    if (ip->type != T_FILE || strcmp(ip->password, password) != 0) {
        iunlockput(ip);
        return -1;
    }
    proc->inode[ip->inum] = 1;
    iunlockput(ip);
    return 0;
}
