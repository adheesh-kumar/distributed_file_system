//
// Starter code for CS 454/654
// You SHOULD change this file
//
#include <iostream>

#include "rpc.h"
#include "debug.h"
INIT_LOG
#include <fuse.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <cstring>
#include <cstdlib>
#include <fcntl.h>
#include <map>


void printAllFlags(int flags)
{
    if(flags & O_CREAT){DLOG("O_CREAT");}
    if(flags & O_EXCL){DLOG("O_EXCL");}
    if(flags & O_WRONLY){DLOG("O_WRONLY");}
    if(flags & O_RDONLY){DLOG("O_RDONLY");}
    if(flags & O_APPEND){DLOG("O_APPEND");}
    if(flags & O_RDWR){DLOG("O_RDWR");}
}


struct FileInfo
{
    //lock?
    bool openWrite = false;
    int openRead = 0;
};

std::map<char *,  struct FileInfo *> files;

// Global state server_persist_dir.
char *server_persist_dir = nullptr;

// Important: the server needs to handle multiple concurrent client requests.
// You have to be careful in handling global variables, especially for updating them.
// Hint: use locks before you update any global variable.

// We need to operate on the path relative to the server_persist_dir.
// This function returns a path that appends the given short path to the
// server_persist_dir. The character array is allocated on the heap, therefore
// it should be freed after use.
// Tip: update this function to return a unique_ptr for automatic memory management.
char *get_full_path(char *short_path) {
    int short_path_len = strlen(short_path);
    int dir_len = strlen(server_persist_dir);
    int full_len = dir_len + short_path_len + 1;

    char *full_path = (char *)malloc(full_len);

    // First fill in the directory.
    strcpy(full_path, server_persist_dir);
    // Then append the path.
    strcat(full_path, short_path);
    //DLOG("Full path: %s\n", full_path);

    return full_path;
}

// - 0

int *numDecode(int n, int l = 0, int *size = NULL)
{
    if (n == 0)
    {
        *size = l;
        return new int[l];
    }
    else
    {
        int len = 0;
        int dig = n % 10;
        
        int *a = numDecode(n / 10, l + 1, &len);

        if (size)
        {
            *size = len;
        }

        a[len - (l + 1)] = dig - 1;
        
        return a;
    }
}

int *createArgTypes(int is_inputN, int is_outputN, int is_arrayN, int typeN, int n)
{
    int *is_input = numDecode(is_inputN);
    int *is_output = numDecode(is_outputN);
    int *is_array = numDecode(is_arrayN);
    int *type = numDecode(typeN);

    int *argTypes = new int[n+1];

    for (int i = 0; i < n; i++)
    {
        argTypes[i] = 
            (is_input[i] * (1u << ARG_INPUT)) |
            (is_output[i] * (1u << ARG_OUTPUT)) |
            (is_array[i] * (1u << ARG_ARRAY)) |
            (type[i] << 16u) |
            (is_array[i] * 1u);
                    
    }
    
    delete []is_input;
    delete []is_output;
    delete []is_array;
    delete []type;

    argTypes[n] = 0;
    return argTypes;
}

// The server implementation of getattr.
int watdfs_getattr(int *argTypes, void **args) {
    // Get the arguments.
    // The first argument is the path relative to the mountpoint.
    char *short_path = (char *)args[0];
    // The second argument is the stat structure, which should be filled in
    // by this function.
    struct stat *statbuf = (struct stat *)args[1];
    // The third argument is the return code, which should be set be 0 or -errno.
    int *ret = (int *)args[2];

    // Get the local file name, so we call our helper function which appends
    // the server_persist_dir to the given path.
    char *full_path = get_full_path(short_path);

    // Initially we set the return code to be 0.
    *ret = 0;

    // TODO: Make the stat system call, which is the corresponding system call needed
    // to support getattr. You should use the statbuf as an argument to the stat system call.
    //(void)statbuf;

    //DLOG("pathname, %s", full_path);
    //DLOG("before statbuf values: '%d', '%d', '%d', '%d', '%d'", statbuf->st_atim, statbuf->st_mode, statbuf->st_blocks, statbuf->st_size, statbuf->st_ino);
    // Let sys_ret be the return code from the stat system call.
    int sys_ret = stat(full_path, statbuf);
    //DLOG("after statbuf values: '%d', '%d', '%d', '%d', '%d'", statbuf->st_atim, statbuf->st_mode, statbuf->st_blocks, statbuf->st_size, statbuf->st_ino);
    //DLOG("ERRNO: '%d", errno);


    //DLOG("GETATTR RETURN VALUE === '%d'", sys_ret);

    if (sys_ret < 0) {
        // If there is an error on the system call, then the return code should
        // be -errno.
        //DLOG("GET ATTR ALSO FUCKED");
        *ret = -errno;
    }

    // Clean up the full path, it was allocated on the heap.
    free(full_path);

    //DLOG("Returning code: %d", *ret);
    
    // The RPC call succeeded, so return 0.
    return 0;
}
int watdfs_mknod(int *argTypes, void **args)
{
    // Get the arguments.
    // The first argument is the path relative to the mountpoint.
    char *short_path = (char *)args[0];
    mode_t *mode = (mode_t *)args[1];
    dev_t *dev = (dev_t *)args[2];
    int *ret = (int *)args[3];

    // Get the local file name, so we call our helper function which appends
    // the server_persist_dir to the given path.
    char *full_path = get_full_path(short_path);

    // Initially we set the return code to be 0.
    *ret = 0;

    //DLOG("MODE ---- DEV:    '%d'  - '%ld'", *mode, *dev);
    // Let sys_ret be the return code from the stat system call.
    int sys_ret = mknod(full_path, *mode, *dev);
    //DLOG("MKNOD RETURN VALUE === '%d'", sys_ret);

    if (sys_ret < 0) {

        //DLOG("mknod failed pussy\n");
        // If there is an error on the system call, then the return code should
        // be -errno.
        *ret = -errno;
    }

    // Clean up the full path, it was allocated on the heap.
    free(full_path);

    //DLOG("Returning code: %d", *ret);
    // The RPC call succeeded, so return 0.
    return 0;
}
int watdfs_open(int *argTypes, void **args)
{
    char *short_path = (char *)args[0];
    struct fuse_file_info *fi = (struct fuse_file_info *)args[1];
    int *ret = (int *)args[2];
    char *full_path = get_full_path(short_path);
    *ret = 0;

    if (files.count(short_path) == 0)
    {
        DLOG("Initalize server file context");
        files[short_path] = (struct FileInfo *)malloc(sizeof(struct FileInfo));
        files[short_path]->openRead = 0;
        files[short_path]->openWrite = false;
    }
        DLOG("count %d", files.count(short_path));

    printAllFlags(fi->flags);

    int check = (fi->flags & O_ACCMODE);

    if (check & O_WRONLY || check & O_RDWR)
    {
        DLOG("WRITE FLAG IN OPEN");
        if (files[short_path]->openWrite)
        {
            DLOG("FILE already open");
            //*ret = -EACCES;
            //return 0;
        }

        files[short_path]->openWrite = true;
        files[short_path]->openRead++;
        check = O_RDWR;
    }
    else
    {
        files[short_path]->openRead++;
        check = O_RDONLY;
    }

    //check if asked and allowed to write
    int sys_ret = open(full_path, check);


    //DLOG("OPEN RETURN VALUE === '%d'", sys_ret);
    if (sys_ret < 0) {
        // If there is an error on the system call, then the return code should
        // be -errno.
        //DLOG("OPEN ALSO FUCKED");
        *ret = -errno;
    }
    else
    {
        fi->flags = check;
        fi->fh = sys_ret;
    }

    // Clean up the full path, it was allocated on the heap.
    free(full_path);

    //DLOG("Returning code: %d", *ret);
    // The RPC call succeeded, so return 0.
    return 0;


}
int watdfs_release(int *argTypes, void **args)
{
    char *short_path = (char *)args[0];
    struct fuse_file_info *fi = (struct fuse_file_info *)args[1];
    int *ret = (int *)args[2];
    char *full_path = get_full_path(short_path);
    *ret = 0;


    int sys_ret = close(fi->fh);


    DLOG("count %d", files.count(short_path));

    /*if (files[short_path]->openWrite)
    {
        DLOG("OPEN FOR WRITE AGAIN");
        files[short_path]->openWrite = false;
    }*/
    //DLOG("RELEASE RETURN VALUE === '%d'", sys_ret);
    if (sys_ret < 0) {
        // If there is an error on the system call, then the return code should
        // be -errno.
        //DLOG("release ALSO FUCKED");
        *ret = -errno;
    }

    // Clean up the full path, it was allocated on the heap.
    free(full_path);

    //DLOG("Returning code: %d", *ret);
    // The RPC call succeeded, so return 0.
    return 0;

}
int watdfs_read(int *argTypes, void **args)
{

    char *short_path = (char *)args[0];
    DLOG("count %d", files.count(short_path));
    void *buf = (void *)args[1];
    size_t *size = (size_t *)args[2];
    off_t *offset = (off_t *)args[3];
    struct fuse_file_info *fi = (struct fuse_file_info *)args[4];
    int *ret = (int *)args[5];

    char *full_path = get_full_path(short_path);
    *ret = 0;

    int sys_ret = pread(fi->fh, buf, *size, *offset);

    if (sys_ret < 0) {
        //DLOG("read ALSO FUCKED");
        *ret = -errno;
    }
    else if (sys_ret > 0)
    {
        *ret = sys_ret;
    }

    free(full_path);

    //DLOG("Returning code: %d", *ret);
    return 0;
}
int watdfs_write(int *argTypes, void **args)
{
    char *short_path = (char *)args[0];
    void *buf = (void *)args[1];
    size_t *size = (size_t *)args[2];
    off_t *offset = (off_t *)args[3];
    struct fuse_file_info *fi = (struct fuse_file_info *)args[4];
    int *ret = (int *)args[5];

    char *full_path = get_full_path(short_path);
    *ret = 0;

    int sys_ret = pwrite(fi->fh, buf, *size, *offset);

    if (sys_ret < 0) {
        //DLOG("write ALSO FUCKED");
        *ret = -errno;
    }

    *ret = sys_ret;

    free(full_path);

    //DLOG("Returning code: %d", *ret);
    return 0;
}
int watdfs_truncate(int *argTypes, void **args)
{
    char *short_path = (char *)args[0];
    off_t *newsize = (off_t *)args[1];
    int *ret = (int *)args[2];
    char *full_path = get_full_path(short_path);
    *ret = 0;

    int sys_ret = truncate(full_path, *newsize);
    
    if (sys_ret < 0) {
        //DLOG("read ALSO FUCKED");
        *ret = -errno;
    }

    free(full_path);

    //DLOG("Returning code: %d", *ret);
    return 0;
}
int watdfs_fsync(int *argTypes, void **args)
{
    char *short_path = (char *)args[0];
    struct fuse_file_info *fi = (struct fuse_file_info *) args[1];
    int *ret = (int *)args[2];
    char *full_path = get_full_path(short_path);
    *ret = 0;

    int sys_ret = fsync(fi->fh);
    if (sys_ret < 0) {
        //DLOG("fsync ALSO FUCKED");
        *ret = -errno;
    }

    free(full_path);

    //DLOG("Returning code: %d", *ret);
    return 0;}
int watdfs_utimensat(int *argTypes, void **args)
{
    char *short_path = (char *)args[0];
    struct timespec *ts = (struct timespec *)args[1];
    int *ret = (int *)args[2];
    char *full_path = get_full_path(short_path);
    *ret = 0;

    //DLOG("server recieved - atime: %d, mtime: %d", ts[0], ts[1]);

    int sys_ret = utimensat(0, full_path, ts, 0);

    //DLOG("UTIMENS RETURN VALUE === '%d'", sys_ret);
    if (sys_ret < 0) {
        // If there is an error on the system call, then the return code should
        // be -errno.
        //DLOG("utimens ALSO FUCKED");
        *ret = -errno;
    }

    // Clean up the full path, it was allocated on the heap.
    free(full_path);

    //DLOG("Returning code: %d", *ret);
    // The RPC call succeeded, so return 0.
    return 0;


}






// The main function of the server.
int main(int argc, char *argv[]) {
    // argv[1] should contain the directory where you should store data on the
    // server. If it is not present it is an error, that we cannot recover from.
    if (argc != 2) {
        // In general, you shouldn't print to stderr or stdout, but it may be
        // helpful here for debugging. Important: Make sure you turn off logging
        // prior to submission!
        // See watdfs_client.cpp for more details
        // # ifdef PRINT_ERR
        // std::cerr << "Usage:" << argv[0] << " server_persist_dir";
        // #endif
        return -1;
    }
    // Store the directory in a global variable.
    server_persist_dir = argv[1];

    // TODO: Initialize the rpc library by calling `rpcServerInit`.
    // Important: `rpcServerInit` prints the 'export SERVER_ADDRESS' and
    // 'export SERVER_PORT' lines. Make sure you *do not* print anything
    // to *stdout* before calling `rpcServerInit`.
    ////DLOG("Initializing server...");

    int ret = rpcServerInit();
    // TODO: If there is an error with `rpcServerInit`, it maybe useful to have
    // debug-printing here, and then you should return.

    if (ret != OK)
    {
        //DLOG("rpcServerInit failed\n");
        return ret;
    }

    // TODO: Register your functions with the RPC library.
    // Note: The braces are used to limit the scope of `argTypes`, so that you can
    // reuse the variable for multiple registrations. Another way could be to
    // remove the braces and use `argTypes0`, `argTypes1`, etc.
  

    int *argTypes[9] = {
        createArgTypes(211, 122, 221, 224, 3),                 //getattr                        
        createArgTypes(2221, 1112, 2111, 2454, 4),             //mknod  
        createArgTypes(221, 122, 221, 224, 3),                 //open     
        createArgTypes(221, 112, 221, 224, 3),                 //release     
        createArgTypes(212221, 121112, 221121, 225524, 6),     //read                 
        createArgTypes(222221, 111112, 221121, 225524, 6),     //write                 
        createArgTypes(221, 112, 211, 254, 3),                 //trunc     
        createArgTypes(221, 112, 221, 224, 3),                 //fsync     
        createArgTypes(221, 112, 221, 224, 3)                  //utimensat     
        };
    
    char * label[9] = {"getattr", "mknod", "open", "release", "read", "write", "truncate", "fsync", "utimensat"};
    skeleton f[9] = {watdfs_getattr, watdfs_mknod, watdfs_open, watdfs_release, watdfs_read, watdfs_write, watdfs_truncate, watdfs_fsync, watdfs_utimensat};


    for (int i = 0; i < 9; i++)
    {
        ret = rpcRegister((char *) label[i], argTypes[i], f[i]);
        delete[] argTypes[i];
        if (ret < 0) {
            // It may be useful to have debug-printing here.
            return ret;
        }
    }

    
    // We need to register the function with the types and the name.
    /*ret = rpcRegister((char *)"getattr", argTypes0, watdfs_getattr);
    delete[] argTypes;
    if (ret < 0) {
        // It may be useful to have debug-printing here.
        return ret;
    }*/

    ret = rpcExecute();


    /* There are 3 args for the function (see watdfs_client.cpp for more
    // detail).
    int argTypes[4];
    // First is the path.
    argTypes[0] =
        (1u << ARG_INPUT) | (1u << ARG_ARRAY) | (ARG_CHAR << 16u) | 1u;
    // The second argument is the statbuf.
    argTypes[1] =
        (1u << ARG_OUTPUT) | (1u << ARG_ARRAY) | (ARG_CHAR << 16u) | 1u;
    // The third argument is the retcode.
    argTypes[2] = (1u << ARG_OUTPUT) | (ARG_INT << 16u);
    // Finally we fill in the null terminator.
    argTypes[3] = 0;
    */

    // TODO: Hand over control to the RPC library by calling `rpcExecute`.
    // rpcExecute could fail, so you may want to have debug-printing here, and
    // then you should return.
    return ret;
}
