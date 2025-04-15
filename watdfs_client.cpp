//
// Starter code for CS 454/654
// You SHOULD change this file
//
#include "watdfs_client.h"
#include "debug.h"
INIT_LOG

#include "rpc.h"
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
    struct fuse_file_info *client_fi = NULL;
    struct stat *server_stat = NULL; //contains T_client
    time_t Tc;
    struct fuse_file_info *server_fi = NULL;
    bool is_open = false;
};


struct ClientPersist
{
    const char *cache_point;
    time_t t;
    std::map<char *,  struct FileInfo *> cached_files;

};

// SETUP AND TEARDOWN
void *watdfs_cli_init(struct fuse_conn_info *conn, const char *path_to_cache,
                      time_t cache_interval, int *ret_code) {
    // TODO: set up the RPC library by calling `rpcClientInit`.

    int rpc = rpcClientInit();
    
    if (rpc != OK)
    {
        *ret_code = NOT_INIT;
        return  NULL;
    }

    // TODO: check the return code of the `rpcClientInit` call
    // `rpcClientInit` may fail, for example, if an incorrect port was exported.

    // It may be useful to print to stderr or stdout during debugging.
    // Important: Make sure you turn off logging prior to submission!
    // One useful technique is to use pre-processor flags like:
    // # ifdef PRINT_ERR
    // std::cerr << "Failed to initialize RPC Client" << std::endl;
    // #endif
    // Tip: Try using a macro for the above to minimize the debugging code.

    // TODO Initialize any global state that you require for the assignment and return it.
    // The value that you return here will be passed as userdata in other functions.
    // In A1, you might not need it, so you can return `nullptr`.

    
    void *userdata = new struct ClientPersist;

   // struct ClientPersist *fill = get_userdata(userdata);
    



    struct ClientPersist *check = (struct ClientPersist *) userdata;
    
    check->cache_point = path_to_cache;
    check->t = cache_interval;
    
    

    // TODO: save `path_to_cache` and `cache_interval` (for A3).

    // TODO: set `ret_code` to 0 if everything above succeeded else some appropriate
    // non-zero value.
    *ret_code = 0;


    // Return pointer to global state data.
    return userdata;
}

void watdfs_cli_destroy(void *userdata) {
    // TODO: clean up your userdata state.
    //clean up dynamic array of FileDescriptors
    //clean up arr of stat



    // TODO: tear down the RPC library by calling `rpcClientDestroy`.
    rpcClientDestroy();
}

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

int *createArgTypes(int is_inputN, int is_outputN, int is_arrayN, int typeN, int n, int *arrayLen)
{
    int *is_input = numDecode(is_inputN);
    int *is_output = numDecode(is_outputN);
    int *is_array = numDecode(is_arrayN);
    int *type = numDecode(typeN);

    int *argTypes = new int[n+1];

    int arrI = 0;
    for (int i = 0; i < n; i++)
    {
        argTypes[i] = 
            (is_input[i] * (1u << ARG_INPUT)) |
            (is_output[i] * (1u << ARG_OUTPUT)) |
            (is_array[i] * (1u << ARG_ARRAY)) |
            (type[i] << 16u) |
            (is_array[i] * (uint) arrayLen[arrI]);

        if (is_array[i] == 1)
        {
            arrI += 1;
        }
    }
    
    delete []is_input;
    delete []is_output;
    delete []is_array;
    delete []type;

    argTypes[n] = 0;
    return argTypes;
}

// server
int original_getattr(const char *path, struct stat *statbuf) 
{
    // SET UP THE RPC CALL
    
    
    // getattr has 3 arguments.
    int ARG_COUNT = 3;

    // Allocate space for the output arguments.
    void **args = new void*[ARG_COUNT];

    // The path has string length (strlen) + 1 (for the null character).
    int pathlen = strlen(path) + 1;
    int arrays[2] = {pathlen, sizeof(struct stat)};

    int *argTypes = createArgTypes(211, 122, 221, 224, ARG_COUNT, arrays);

    // Fill in the arguments
    // For arrays the argument is the array pointer, not a pointer to a pointer.
    args[0] = (void *)path;
    args[1] = (void *)statbuf;
    int retcode = 0;
    args[2] =(void *) &retcode;

    // MAKE THE RPC CALL
    int rpc_ret = rpcCall((char *)"getattr", argTypes, args);


    // HANDLE THE RETURN
    // The integer value watdfs_cli_getattr will return.
    int fxn_ret = 0;
    if (rpc_ret < 0) {
        
        // Something went wrong with the rpcCall, return a sensible return
        // value. In this case lets return, -EINVAL
        fxn_ret = -EINVAL;
    } else {
        // Our RPC call succeeded. However, it's possible that the return code
        // from the server is not 0, that is it may be -errno. Therefore, we
        // should set our function return value to the retcode from the server.

        // TODO: set the function return value to the return code from the server.
        fxn_ret = retcode;
        
    }

    if (fxn_ret < 0) {
        // If the return code of watdfs_cli_getattr is negative (an error), then 
        // we need to make sure that the stat structure is filled with 0s. Otherwise,
        // FUSE will be confused by the contradicting return values.
        memset(statbuf, 0, sizeof(struct stat));
    }

    // Clean up the memory we have allocated.
    delete []args;

    // Finally return the value we got from the server.
    return fxn_ret;
}
int original_mknod(const char *path, mode_t mode, dev_t dev) 
{

    int ARG_COUNT = 4;
    void **args = new void*[ARG_COUNT];
    int pathlen = strlen(path) + 1;
    int retcode = 0;


    //create RPC args
    int arrays[1] = {pathlen};
    int *argTypes = createArgTypes(2221, 1112, 2111, 2454, ARG_COUNT, arrays);
    args[0] = (void *)path;
    args[1] = (void *)&mode;
    args[2] = (void *)&dev;
    args[3] =(void *) &retcode;
    //end args


    // MAKE THE RPC CALL
    int rpc_ret = rpcCall((char *)"mknod", argTypes, args);

    


    // HANDLE THE RETURN
    int fxn_ret = 0;
    if (rpc_ret < 0) {
        fxn_ret = -EINVAL;
    } 
    else 
    {
        fxn_ret = retcode;
    }


    delete []args;
    return fxn_ret;
}
int original_open(const char *path, struct fuse_file_info *fi) 
{
    /*

    A3:

    1. check if fi exists at server?

    2. server rpc call open with O_READONLY
    3. open/create a client copy with truncate flag - persist in userdata->fh
    4. getattr(server fh)
    5. read server->fh *buffer
    6. write *buffer into userdata->fh
    7. set userdata

    */

    int ARG_COUNT = 3;
    int retcode = 0;
    void **args = new void*[ARG_COUNT];
    int pathlen = strlen(path) + 1;
    
    int arrays[2] = {pathlen, sizeof(struct fuse_file_info)};
    int *argTypes = createArgTypes(221, 122, 221, 224, ARG_COUNT, arrays);

    // Fill in the arguments
    args[0] = (void *)path;
    args[1] = (void *)fi;
    args[2] =(void *) &retcode;
    //end args

    //DLOG("within the download call");
    // MAKE THE RPC CALL
    int rpc_ret = rpcCall((char *)"open", argTypes, args);


    //DLOG("rpc call return val: %d", rpc_ret);



    // HANDLE THE RETURN
    int fxn_ret = 0;
    if (rpc_ret < 0) {
        fxn_ret = -EINVAL;
    } 
    else 
    {
        fxn_ret = retcode;
    }

    delete []args;
    return fxn_ret;
}
int original_release(const char *path, struct fuse_file_info *fi) 
{
    
    int ARG_COUNT = 3;
    int retcode = 0;
    void **args = new void*[ARG_COUNT];
    int pathlen = strlen(path) + 1;

    int arrays[2] = {pathlen, sizeof(struct fuse_file_info)};
    int *argTypes = createArgTypes(221, 112, 221, 224, ARG_COUNT, arrays);

    // Fill in the arguments
    args[0] = (void *)path;
    args[1] = (void *)fi;
    args[2] =(void *) &retcode;
    //end args

    // MAKE THE RPC CALL
    int rpc_ret = rpcCall((char *)"release", argTypes, args);



    // HANDLE THE RETURN
    int fxn_ret = 0;
    if (rpc_ret < 0) {
        fxn_ret = -EINVAL;
    } 
    else 
    {
        fxn_ret = retcode;
    }

    delete []args;
    return fxn_ret;
}
int original_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) 
{
    //fi passed in is server-side 
    //struct ClientPersist *client = (struct ClientPersist *) userdata;



    //      A3:
    // 1. check client->files->server_fi->fh == fi->fh
    //      a. true: check is_fresh?
    //          -true  -- syscall read(client->files->client_fi)
    //          -false --

    //      b. false: open?
    //          - rpc call
    //          - syscall write(buf into client->files->client_fi)
    















    
    // Read size amount of data at offset of file into buf.
    int nCalls = size / MAX_ARRAY_LEN + 1;
    int pathlen = strlen(path) + 1;

    //size_t iterSize = size;
    
    int ARG_COUNT = 6;
    void **args = new void*[ARG_COUNT];


    int fxn_ret = 0;
    int retcode = 0;


    size_t requestFor = size;
    off_t newOffset = offset;
    char * newBuf = buf;


    args[0] = (void *)path;
    args[4] = (void *)fi;


    for (int i = 0; i < nCalls; i++)
    {


        if (requestFor > MAX_ARRAY_LEN)
        {
            requestFor = MAX_ARRAY_LEN;
        }


        int arrays[3] = {pathlen, requestFor, sizeof(struct fuse_file_info)};
        int *argTypes = createArgTypes(212221, 121112, 221121, 225524, ARG_COUNT, arrays);

        // Fill in the arguments
        args[1] = (void *)newBuf;
        args[2] = (void *)&requestFor;
        args[3] = (void *)&newOffset;
        args[5] =(void *) &retcode;
        //end args
        

        // MAKE THE RPC CALL
        int rpc_ret = rpcCall((char *)"read", argTypes, args);

        
        // HANDLE THE RETURN
        if (rpc_ret < 0) {
            fxn_ret = -EINVAL;
            return fxn_ret;
        } 
        else 
        {
            fxn_ret += retcode; //ret
            newOffset += retcode; //wheren in fi to start
            newBuf+= retcode;  //where to start reading into
            //requestFor-= retcode;

            if (retcode < 0)
            {
                //error
                return retcode;
            }
            else if (retcode < requestFor || retcode == 0)
            {
                return fxn_ret;
            }

            requestFor = size - fxn_ret;


        }
    }

    delete []args;
    return fxn_ret;
}
int original_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) 
{
    
    // Read size amount of data at offset of file into buf.
    int nCalls = size / MAX_ARRAY_LEN + 1;
    int pathlen = strlen(path) + 1;
    //size_t iterSize = size;
    
    int ARG_COUNT = 6;
    void **args = new void*[ARG_COUNT];
    int fxn_ret = 0;
    int retcode = 0;
    size_t requestFor = size;
    off_t newOffset = offset;
    const char *newBuf = buf;

    args[0] = (void *)path;
    args[4] = (void *)fi;

    for (int i = 0; i < nCalls; i++)
    {

        if (requestFor > MAX_ARRAY_LEN)
        {
            requestFor = MAX_ARRAY_LEN;
        }

        int arrays[3] = {pathlen, requestFor, sizeof(struct fuse_file_info)};
        int *argTypes = createArgTypes(222221, 111112, 221121, 225524, ARG_COUNT, arrays);

        // Fill in the arguments
        args[1] = (void *)newBuf;
        args[2] = (void *)&requestFor;
        args[3] = (void *)&newOffset;
        args[5] =(void *) &retcode;
        //end args
        

        // MAKE THE RPC CALL
        int rpc_ret = rpcCall((char *)"write", argTypes, args);

        
        // HANDLE THE RETURN
        if (rpc_ret < 0) {
            fxn_ret = -EINVAL;
            return fxn_ret;
        } 
        else 
        {
            
            fxn_ret += retcode;
            newOffset += retcode;
            newBuf+= retcode;

            if (retcode < 0)
            {
                return retcode;
            }
            else if (retcode < requestFor)
            {
                return fxn_ret;
            }

            requestFor = size - fxn_ret;
        }


    }

    delete []args;
    return fxn_ret;
}
int original_truncate(const char *path, off_t newsize)
{
    
    int ARG_COUNT = 3;
    int retcode = 0;
    void **args = new void*[ARG_COUNT];
    int pathlen = strlen(path) + 1;

    int arrays[1] = {pathlen};
    int *argTypes = createArgTypes(221, 112, 211, 254, ARG_COUNT, arrays);

    // Fill in the arguments
    args[0] = (void *)path;
    args[1] = (void *)&newsize;
    args[2] =(void *) &retcode;
    //end args

    // MAKE THE RPC CALL
    int rpc_ret = rpcCall((char *)"truncate", argTypes, args);


    // HANDLE THE RETURN
    int fxn_ret = 0;
    if (rpc_ret < 0) {
        fxn_ret = -EINVAL;
    } 
    else 
    {
        fxn_ret = retcode;
    }

    delete []args;
    return fxn_ret;
}
int original_fsync(const char *path, struct fuse_file_info *fi) 
{

    int ARG_COUNT = 3;
    int retcode = 0;
    void **args = new void*[ARG_COUNT];
    int pathlen = strlen(path) + 1;
    
    int arrays[2] = {pathlen, sizeof(struct fuse_file_info)};
    int *argTypes = createArgTypes(221, 112, 221, 224, ARG_COUNT, arrays);

    // Fill in the arguments
    args[0] = (void *)path;
    args[1] = (void *)fi;
    args[2] =(void *) &retcode;
    //end args

    
    // MAKE THE RPC CALL
    int rpc_ret = rpcCall((char *)"fsync", argTypes, args);


    // HANDLE THE RETURN
    int fxn_ret = 0;
    if (rpc_ret < 0) {
        fxn_ret = -EINVAL;
    } 
    else 
    {
        fxn_ret = retcode;
    }

    delete []args;
    return fxn_ret;
}
int original_utimensat(const char *path, const struct timespec ts[2]) 
{
    
    int ARG_COUNT = 3;
    int retcode = 0;
    void **args = new void*[ARG_COUNT];
    int pathlen = strlen(path) + 1;

    int sizeTS = 2*sizeof(struct timespec);

    int arrays[2] = {pathlen, sizeTS};
    int *argTypes = createArgTypes(221, 112, 221, 224, ARG_COUNT, arrays);

    // Fill in the arguments
    args[0] = (void *)path;
    args[1] = (void *)ts;
    args[2] =(void *) &retcode;
    //end args


    // MAKE THE RPC CALL
    int rpc_ret = rpcCall((char *)"utimensat", argTypes, args);


    // HANDLE THE RETURN
    int fxn_ret = 0;
    if (rpc_ret < 0) {
        // Something went wrong with the rpcCall, return a sensible return
        // value. In this case lets return, -EINVALreturn -ENOSYS;
        fxn_ret = -EINVAL;
    } 
    else 
    {
        fxn_ret = 0;
    }

    delete []args;
    return fxn_ret;
}

//char *getMapPath(void *userdata, const char *path)

struct FileInfo *getFileInfo(void *userdata, const char *path)
{
    DLOG("getting fileinfo");
    struct ClientPersist *client = (struct ClientPersist *) userdata;

    

    struct FileInfo *ret = (client->cached_files.find((char *)path))->second;
    DLOG("getFI %p", ret);

    DLOG("finish get");


    return ret;
}
void initalizeCacheData(void *userdata, const char *path)
{
    DLOG("begin initalize");
    struct ClientPersist *client = (struct ClientPersist *) userdata;
    char *mapPath = (char *)malloc(strlen(path));
    strcpy(mapPath, path);

    client->cached_files[mapPath] = new struct FileInfo;
    client->cached_files[mapPath]->client_fi = new struct fuse_file_info;
    client->cached_files[mapPath]->server_fi = new struct fuse_file_info;
    client->cached_files[mapPath]->server_stat = new struct stat;
    client->cached_files[mapPath]->is_open = false;

    //free(mapPath);
    DLOG("seg %p", client->cached_files[mapPath]);

    DLOG("finish initalize");
}
bool existsInCache(void *userdata, const char *path)
{
    struct ClientPersist *client = (struct ClientPersist *) userdata;

    if (client->cached_files.count((char *)path) > 0)
    {
        DLOG("EXISTS: count - %d", client->cached_files.count((char *)path));
        return true;
    }

    return false;

}
char *get_full_path(void *userdata, const char *short_path) {
    struct ClientPersist *client = (struct ClientPersist *) userdata;


    int short_path_len = strlen(short_path);
    int dir_len = strlen(client->cache_point);
    int full_len = dir_len + short_path_len + 1;

    char *full_path = (char *)malloc(full_len);

    // First fill in the directory.
    strcpy(full_path, client->cache_point);
    // Then append the path.
    strcat(full_path, short_path);
    //DLOG("Full path: %s\n", full_path);

    return full_path;
}
void printFileInfo(void *userdata, const char *path)
{
    struct ClientPersist *client = (struct ClientPersist *) userdata;
    DLOG("File info for: %s", path);
    if (!existsInCache(userdata, path))
    {
        DLOG("cache data DNE");
        return;
    }

    struct FileInfo *fi = getFileInfo(userdata, path);



    DLOG("is_open: %d", fi->is_open);
    DLOG("cache verify Tc: %d", fi->Tc);
    DLOG("server stats: %p", fi->server_stat);
    DLOG("client fi: %p", fi->client_fi);
    DLOG("server fi: %p", fi->server_fi);
}



int download(void *userdata, const char *path)
{
    struct ClientPersist *client = (struct ClientPersist *) userdata;
    struct FileInfo *file_data = getFileInfo(userdata, path);

    //assume we are set for download except for EACCES


    DLOG("(try to) download file");
    // (try to) open file on server
    int ret = original_open(path, file_data->server_fi);

    //check if server allowed the open
    if (ret < 0)
    {
        return ret;
    }

    //can downlaod now
    //DLOG("Attempt to download now");

    truncate(get_full_path(userdata, path), 0);

    //get latest server stat (stat hold meta data for server)
    original_getattr(path, file_data->server_stat);
    
    //create buffer to read into
    int bufLen = file_data->server_stat->st_size;
    char *buf = new char[bufLen];
    //DLOG("reading open file from server: buflen - %d", bufLen);

    //read from server
    int bytesRead = original_read(path, buf, bufLen, 0, file_data->server_fi);
    if (bytesRead != bufLen)
    {
        //DLOG("SCREAMMMMMMMM improper amount read to local cache: %d", bytesRead);
    }


    char *cache_path = get_full_path(userdata, path);

    //open local file
    int fd = open(cache_path, O_TRUNC | O_WRONLY); //open for write!!

    //write to cache
    int bytesWritten = write(fd, buf, bufLen);
    if (bytesWritten != bufLen)
    {
        //DLOG("SCREAM not reading full file from server: %d", bytesWritten);
    }

    //DLOG("bytes should have been written to CACHE!!!");

    //close both files
    close(fd);

    delete buf;


    return 0;
}

int upload(void *userdata, const char *path)
{
    struct ClientPersist *client = (struct ClientPersist *) userdata;
    struct FileInfo *file_data = getFileInfo(userdata, path);

    //assume we are 100% uploading

    //read local file into buf
    struct stat statbuf;
    stat(get_full_path(userdata, path), &statbuf);

    int bufLen = statbuf.st_size;
    char *buf = new char[bufLen];

    int local_fh = open(get_full_path(userdata, path), O_RDONLY);
    read(local_fh, buf, bufLen);
    close(local_fh);
    //done

    original_open(path, file_data->server_fi);
    int bytesWritten = original_write(path, buf, bufLen, 0, file_data->server_fi);
    original_release(path, file_data->server_fi);

    delete buf;

    return bytesWritten;

}

bool readFreshness(void *userdata, const char *path)
{
    struct ClientPersist *client = (struct ClientPersist *) userdata;
    struct FileInfo *fi = getFileInfo(userdata, path);
    //determine whether or not to download

    DLOG("Checking read freshness");

    if (!existsInCache(userdata, path))
    {
        return true;

    }
    DLOG("is open for reads");

    time_t T;
    time(&T);

    if ((T - fi->Tc) > client->t)
    {
        struct timespec T_client = fi->server_stat->st_mtim;
        original_getattr(path, fi->server_stat);

        if (!(T_client.tv_nsec == fi->server_stat->st_mtim.tv_nsec))
        {
            return false;
        }
        
        time(&(fi->Tc));
    }



    return true;
}

int writeFreshness(void *userdata, const char *path)
{
    return upload(userdata, path);
}

bool isOpen(void *userdata, const char *path)
{
    struct ClientPersist *client = (struct ClientPersist *) userdata;
    if (!existsInCache(userdata, path))
    {
        return false;
    }
    struct FileInfo *fi = getFileInfo(userdata, path);


    return fi->is_open;
}



int watdfs_cli_getattr(void *userdata, const char *path, struct stat *statbuf) //complete
{
    struct ClientPersist *client = (struct ClientPersist *) userdata;
    printFileInfo(userdata, path);


    int ret = stat(get_full_path(userdata, path), statbuf);

    //return attributes of cached file
    if (ret < 0)
    {   
        DLOG("File not found in cache");

        if(original_getattr(path, statbuf) < 0)
        {
            DLOG("File not found on server");
            return -ENOENT;
        }

    

        DLOG("File exists on server, not in cache");
        DLOG("Initializing client persist:");
        initalizeCacheData(userdata, path);
        getFileInfo(userdata, path)->server_fi = O_RDONLY;

        DLOG("getattr: Creating local file");
        close(open(get_full_path(userdata, path), O_CREAT));
        download(userdata, path);
    }
    //else{readFreshness(userdata, path);}

    return stat(get_full_path(userdata, path), statbuf);
}

int watdfs_cli_mknod(void *userdata, const char *path, mode_t mode, dev_t dev) //i think complete
{
    struct ClientPersist *client = (struct ClientPersist *) userdata;
    printFileInfo(userdata, path);

    printAllFlags((int)mode);
    printAllFlags((int)dev);
    //ensure file isn't already cached
    /*if (client->cached_files.count(path) > 0)
    {
        //file exists locally abort
        return -EEXIST;
    }*/
    
    DLOG("mknod: Creating local file");
    int ret = mknod(get_full_path(userdata, path), mode, dev);

    //check if file exists on server
    struct stat buf;
    if (original_getattr(path, &buf) < 0)
    {

        DLOG("mknod: Creating server file, getattr<0");
        //DLOG("DOESNT EXIST ON SERVER, CREATING NOW");
        //create file on server
        original_mknod(path, mode, dev);
    }

    return ret;
}


int watdfs_cli_open(void *userdata, const char *path, struct fuse_file_info *fi) //i think complete
{
    struct ClientPersist *client = (struct ClientPersist *) userdata;
    printFileInfo(userdata, path);

    printAllFlags(fi->flags);

    if (!existsInCache(userdata, path))
    {
        DLOG("Initializing client persist:");
        initalizeCacheData(userdata, path);
    }
    //check that file is not already open locally
    if (isOpen(userdata, path))
    {
        //too many open files
        return -EMFILE;
    }
    struct FileInfo *file = getFileInfo(userdata, path);

    DLOG("seg %p", file);
    file->client_fi = fi;
    DLOG("seg");
    *(file->server_fi) = *fi;

    DLOG("seg");
    //ready to try downloading
    int ret = download(userdata, path);

    if (ret < 0)
    {
        //permission denied
        return ret;
    }


    //DLOG("content should be downloaded from server into cache");

    //set fh for local file
    fi->fh = open(get_full_path(userdata, path), fi->flags);
    
    //set cache map to open
    file->is_open = true;


    return ret;
}

int watdfs_cli_release(void *userdata, const char *path, struct fuse_file_info *fi) //complete
{
    struct ClientPersist *client = (struct ClientPersist *) userdata;
    struct FileInfo *file = getFileInfo(userdata, path);

    //write back if needed
    //writeFreshness(userdata, path);

    //clear status from cache map
    //delete client->cached_files[path]->server_fi;
    //delete client->cached_files[path]->server_stat;
    close(fi->fh);
    original_release(path, file->server_fi);
    file->is_open = false;

    //close client fh
    return 0;
}

// READ AND WRITE DATA
int watdfs_cli_read(void *userdata, const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
    struct ClientPersist *client = (struct ClientPersist *) userdata;

    //update cache if needed
    //readFreshness(userdata, path);

    //read from cache
    return pread(fi->fh, buf, size, offset);
}

int watdfs_cli_write(void *userdata, const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
    struct ClientPersist *client = (struct ClientPersist *) userdata;

    //write to cache
    pwrite(fi->fh, buf, size, offset);

    //send to server if needed
    return writeFreshness(userdata, path);
}

int watdfs_cli_truncate(void *userdata, const char *path, off_t newsize) // complete
{
    struct ClientPersist *client = (struct ClientPersist *) userdata;
    
    //write to cache
    truncate(get_full_path(userdata, path), newsize);

    //send to server if needed
    return 0; //writeFreshness(userdata, path);
}
int watdfs_cli_fsync(void *userdata, const char *path,
                     struct fuse_file_info *fi)
{
    struct ClientPersist *client = (struct ClientPersist *) userdata;
    struct FileInfo *file = getFileInfo(userdata, path);

    if((fi->flags & O_ACCMODE) & O_RDONLY)
    {
        return -EACCES;
    }

    upload(userdata, path);
    original_getattr(path, file->server_stat);
    time(&(file->Tc));

    return 0;
}

int watdfs_cli_utimensat(void *userdata, const char *path,
                       const struct timespec ts[2])
{
    struct ClientPersist *client = (struct ClientPersist *) userdata;

   // if (isOpen(userdata, path))
    {

    }
    return original_utimensat(path, ts);
}