#define _GNU_SOURCE
#define _FILE_OFFSET_BITS 64
#define FUSE_USE_VERSION 26

#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <fuse.h>

struct redirfs_options {
  char* target;
  char* mountpoint;
  int mountpoint_mode;
};

static struct redirfs_options options;

static int opr_getattr(const char *path, struct stat *stbuf){
  memset(stbuf, 0, sizeof(struct stat));
  stbuf->st_uid = getuid();
  stbuf->st_gid = getgid();

  if( S_ISDIR(options.mountpoint_mode)
   && path[0] == '/' && path[1] == 0
  ){
    stbuf->st_mode = S_IFDIR | 0550;
    stbuf->st_nlink = 1;
  }else if(options.target[0] == '|'){
    stbuf->st_mode = S_IFREG | 0220;
    stbuf->st_nlink = 1;
  }else{
    stbuf->st_mode = S_IFLNK | 0440;
    stbuf->st_nlink = 1;
  }

  return 0;
}

static int opr_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi){
  (void)offset;
  (void)fi;
  (void)path;

  filler(buf, ".", NULL, 0);
  filler(buf, "..", NULL, 0);

  return 0;
}

static int opr_readlink(const char* path, char* link, size_t size){
  (void)path;
  if(sizeof(options.target) >= size)
    return -ENOMEM;
  if(options.target[0] == '|')
    return -EINVAL;
  strcpy(link, options.target);
  return 0;
}

static int opr_open(const char *path, struct fuse_file_info *fi){
  fi->fh = -1;
  if(options.target[0] != '|')
    return -EINVAL;
  fi->nonseekable = true;
  int io[2];
  if(pipe2(io,O_CLOEXEC) == -1)
    goto pipefail_1;
  int proccheck[2];
  if(pipe2(proccheck,O_CLOEXEC) == -1)
    goto pipefail_2;
  int ret = fork();
  if(ret == -1)
    goto forkfail;
  if(ret){
    close(io[0]);
    close(proccheck[1]);
    fi->fh = io[1];
    fi->direct_io = io[1];
    unsigned char ok = true;
    while(read(proccheck[0],&ok,1) == -1 && errno == EINTR);
    close(proccheck[0]);
    if(!ok){
      close(io[1]);
      return -EINVAL;
    }
  }else{
    close(io[1]);
    close(proccheck[0]);
    if(dup2(io[0],STDIN_FILENO) == -1)
      goto execfail;
    close(io[0]);
    if(setenv("FILE",path+1,true) == -1)
      goto execfail;
    if(fcntl(STDIN_FILENO, F_SETFD, 0) == -1) // remove O_CLOEXEC
      goto execfail;
    execl("/bin/sh", "sh", "-c", options.target+1, 0);
  execfail:
    while( write(proccheck[1], (char[]){0}, 1) == -1 && errno == EINTR );
    exit(1);
  }
  return 0;
forkfail:
  close(proccheck[0]);
  close(proccheck[1]);
pipefail_2:
  close(io[0]);
  close(io[1]);
pipefail_1:
  return -EINVAL;
}

static int ops_truncate(const char* path, off_t offset){
  (void)path;
  (void)offset;
  return 0;
}

static int opr_close(const char *path, struct fuse_file_info *fi){
  (void)path;
  close(fi->fh);
  return 0;
}

static int opr_write(const char* path, const char* buf, size_t size, off_t offset, struct fuse_file_info* fi){
  (void)path;
  (void)offset;
  if(fi->fh == (unsigned)-1)
    return size;
  size_t s = 0;
  while(s < size){
    ssize_t res = write(fi->fh, buf, size);
    if(res == -1 || res == 0){
      if(res && errno == EINTR) continue;
      close(fi->fh);
      fi->fh = -1;
      fi->direct_io = -1;
      return res ? -errno : (int)s;
    }
    s += size;
  }
  return s;
}

static struct fuse_operations operations = {
  .getattr = opr_getattr,
  .readdir = opr_readdir,
  .readlink = opr_readlink,
  .truncate = ops_truncate,
  .open = opr_open,
  .write = opr_write,
  .release = opr_close,
};

static int opt_proc(void *data, const char *arg, int key, struct fuse_args *outargs){
  (void)data;
  (void)key;
  (void)outargs;
  if( key == FUSE_OPT_KEY_NONOPT && !options.target ){
    options.target = strdup(arg);
    return 0;
  }else if(!options.mountpoint){
    options.mountpoint = strdup(arg);
    struct stat path_stat;
    stat(arg, &path_stat);
    options.mountpoint_mode = path_stat.st_mode;
    if( S_ISREG(options.mountpoint_mode) && options.target[0] != '|' ){
      puts("Error: Can't mount symlink over regular file, consider creating a symlink yourself with ln -s instead.");
      exit(1);
    }
  }
  return 1;
}

int main(int argc, char* argv[]){
  struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
  fuse_opt_parse(&args, 0, 0, opt_proc);
  return fuse_main(args.argc, args.argv, &operations, 0);
}
