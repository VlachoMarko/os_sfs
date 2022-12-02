#define FUSE_USE_VERSION 26

#include <errno.h>
#include <fuse.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

#include "sfs.h"
#include "diskio.h"


static const char default_img[] = "test.img";

/* Options passed from commandline arguments */
struct options {
    const char *img;
    int background;
    int verbose;
    int show_help;
    int show_fuse_help;
} options;


#define log(fmt, ...) \
    do { \
        if (options.verbose) \
            printf(" # " fmt, ##__VA_ARGS__); \
    } while (0)


/* libfuse2 leaks, so let's shush LeakSanitizer if we are using Asan. */
const char* __asan_default_options() { return "detect_leaks=0"; }


/*
 * This is a helper function that is optional, but highly recomended you
 * implement and use. Given a path, it looks it up on disk. It will return 0 on
 * success, and a non-zero value on error (e.g., the file did not exist).
 * The resulting directory entry is placed in the memory pointed to by
 * ret_entry. Additionally it can return the offset of that direntry on disk in
 * ret_entry_off, which you can use to update the entry and write it back to
 * disk (e.g., rmdir, unlink, truncate, write).
 *
 * You can start with implementing this function to work just for paths in the
 * root entry, and later modify it to also work for paths with subdirectories.
 * This way, all of your other functions can use this helper and will
 * automatically support subdirectories. To make this function support
 * subdirectories, we recommend you refactor this function to be recursive, and
 * take the current directory as argument as well. For example:
 *
 *  static int get_entry_rec(const char *path, const struct sfs_entry *parent,
 *                           size_t parent_nentries, blockidx_t parent_blockidx,
 *                           struct sfs_entry *ret_entry,
 *                           unsigned *ret_entry_off)
 *
 * Here parent is the directory it is currently searching (at first the rootdir,
 * later the subdir). The parent_nentries tells the function how many entries
 * there are in the directory (SFS_ROOTDIR_NENTRIES or SFS_DIR_NENTRIES).
 * Finally, the parent_blockidx contains the blockidx of the given directory on
 * the disk, which will help in calculating ret_entry_off.
 */
static int get_entry_rec(const char *path, const struct sfs_entry *parent,
                            size_t parent_nentries, blockidx_t parent_blockidx,
                            struct sfs_entry *ret_entry)
{
    /* Get the next component of the path. Make sure to not modify path if it is
     * the value passed by libfuse (i.e., make a copy). Note that strtok
     * modifies the string you pass it. */

    /* Allocate memory for the rootdir (and later, subdir) and read it from disk */

    /* Loop over all entries in the directory, looking for one with the name
     * equal to the current part of the path. If it is the last part of the
     * path, return it. If there are more parts remaining, recurse to handle
     * that subdirectory. */

    log("path in entry: %s",path);
    
    int res = 1;    
    char pathCopy[59];    
    void *lastDir = NULL;
    const char slash[2] = "/";
    
    strcpy(pathCopy, path);
    lastDir = strtok(pathCopy, slash);

    if(lastDir != NULL){
        void *temp = lastDir;

        while(temp != NULL){
            lastDir = temp;
            temp = strtok(NULL, slash);
        }
    }

    struct sfs_entry dir[parent_nentries];
      
    if(parent_nentries == SFS_ROOTDIR_NENTRIES){

        strcpy(pathCopy, path);
        void *childDir = strtok(pathCopy, slash);

        disk_read(dir, SFS_ROOTDIR_SIZE, SFS_ROOTDIR_OFF);
        
        for (unsigned int i=0; i < parent_nentries; i++){
            if(strlen(dir[i].filename) > 0 && strcmp(dir[i].filename, lastDir) == 0){
            
                res = 0;

                if(dir[i].size & SFS_DIRECTORY){
                    log("found dir in root: %s", (char *) lastDir);

                    disk_read(ret_entry, sizeof(struct sfs_entry), SFS_ROOTDIR_OFF + i * sizeof(struct sfs_entry));
                    return res;
                }        
                else {
                    log("found file in root: %s", (char *) lastDir);
                    disk_read(ret_entry, sizeof(struct sfs_entry), SFS_ROOTDIR_OFF + i * sizeof(struct sfs_entry));
                    return res;
                }   
            }  
        } 
        for (unsigned int i=0; i < parent_nentries; i++){
            if(strlen(dir[i].filename) > 0 && strcmp(dir[i].filename, childDir) == 0){
                
                log("goto childDir %s ", (char *) childDir);

                disk_read(ret_entry, sizeof(struct sfs_entry), SFS_DATA_OFF + dir[i].first_block * SFS_BLOCK_SIZE + i * sizeof(struct sfs_entry));

                //struct sfs_entry *tempParent = malloc(64);
                //disk_read(tempParent, sizeof(struct sfs_entry), SFS_ROOTDIR_OFF + i * sizeof(struct sfs_entry));


                res = get_entry_rec(path, NULL, SFS_DIR_NENTRIES, dir[i].first_block, ret_entry);
                return res;
            }
        }

        return 1;
    }
    else if (parent_nentries == SFS_DIR_NENTRIES) {

        disk_read(dir, SFS_DIR_SIZE, SFS_DATA_OFF + parent_blockidx * SFS_BLOCK_SIZE);
        
        for(unsigned int i=0; i<parent_nentries; i++){
            
            if(strlen(dir[i].filename) > 0 && strcmp(dir[i].filename, lastDir) == 0){

                res = 0;

                if(dir[i].size & SFS_DIRECTORY){
                    log("found dir %s in subdir", (char *) lastDir);
                    log("reading block %x, at image offset:%x", dir[i].first_block, SFS_DATA_OFF + dir[i].first_block * SFS_BLOCK_SIZE);

                    // disk_read(parent, sizeof(struct sfs_entry), SFS_DATA_OFF + i * sizeof(struct sfs_entry) + parent_blockidx * SFS_BLOCK_SIZE);
                    // disk_read(ret_entry, sizeof(struct sfs_entry), SFS_DATA_OFF + i * sizeof(struct sfs_entry) + parent_blockidx * SFS_BLOCK_SIZE);
                    return res;
                }        
                else {
                    log("found file %s in subdir", (char *) lastDir);
                    //disk_read(ret_entry, sizeof(struct sfs_entry), SFS_DATA_OFF + parent_blockidx * SFS_BLOCK_SIZE + i);
                    return res;
                }   
            }

        }

        // for(unsigned int i=0; i<parent_nentries; i++){
        //     if(strlen(dir[i].filename) > 0 && strcmp(dir[i].filename, childDir) == 0){
                
        //         log("goto childDir again");
        //         struct sfs_entry parent[16];
        //         disk_read(parent, SFS_DIR_SIZE, SFS_DATA_OFF + dir[i].first_block * SFS_BLOCK_SIZE);  

        //         res = get_entry_rec(path, parent, SFS_DIR_NENTRIES, dir[i].first_block, ret_entry);
        //         return res;
        //     }
        // }


        return 1;
    }
    else {
        log("not correct parentnentries");
        return 1;
    }

    // log("path after: %s", (char *) path);

    return res;
}


/*
 * Retrieve information about a file or directory.
 * You should populate fields of `stbuf` with appropriate information if the
 * file exists and is accessible, or return an error otherwise.
 *
 * For directories, you should at least set st_mode (with S_IFDIR) and st_nlink.
 * For files, you should at least set st_mode (with S_IFREG), st_nlink and
 * st_size.
 *
 * Return 0 on success, < 0 on error.
 */
static int sfs_getattr(const char *path,
                       struct stat *st)
{
    int res = -ENOENT;

    log("getattr %s\n", path);

    memset(st, 0, sizeof(struct stat));
    /* Set owner to user/group who mounted the image */
    st->st_uid = getuid();
    st->st_gid = getgid();
    /* Last accessed/modified just now */
    st->st_atime = time(NULL);
    st->st_mtime = time(NULL);

    char pathCopy[59];
    strcpy(pathCopy, path);
    const char slash[2] = "/";
    void *lastDir;

    lastDir = strtok(pathCopy, slash);
    lastDir = strtok(NULL, slash);


    if (strcmp(path, "/") == 0) {
        st->st_mode = S_IFDIR | 0755;
        st->st_nlink = 2;
        res = 0;
    }else if(lastDir != NULL){

        log("not rootpath");
        struct sfs_entry *entry = malloc(64);

        if(get_entry_rec(path, NULL, SFS_ROOTDIR_NENTRIES, 0, entry) > 0 ){
            return -ENOENT;
        }



        if(entry->size & SFS_DIRECTORY){
            log("is dir");
            st->st_mode = S_IFDIR | 0755;
            st->st_nlink = 2;
        }
        else {
            log("is file");
            st->st_mode = S_IFREG | 0755;
            st->st_nlink = 1;
            st->st_size = entry->size & SFS_SIZEMASK;
        }   
        res = 0;
    }
    else {
        log("rootpath");

        struct sfs_entry rootdir[SFS_ROOTDIR_NENTRIES];
        disk_read(rootdir, SFS_ROOTDIR_SIZE, SFS_ROOTDIR_OFF);

        for (unsigned int i=0; i < SFS_ROOTDIR_NENTRIES; i++){

            if(strcmp(rootdir[i].filename, &path[1]) == 0){
                if(rootdir[i].size & SFS_DIRECTORY){
                    st->st_mode = S_IFDIR | 0755;
                    st->st_nlink = 2;
                }
                else {
                    st->st_mode = S_IFREG | 0755;
                    st->st_nlink = 1;
                    st->st_size = rootdir[i].size & SFS_SIZEMASK;
                }

                res = 0;
            }
        }
           
    }

    return res;
}


/*
 * Return directory contents for `path`. This function should simply fill the
 * filenames - any additional information (e.g., whether something is a file or
 * directory) is later retrieved through getattr calls.
 * Use the function `filler` to add an entry to the directory. Use it like:
 *  filler(buf, <dirname>, NULL, 0);
 * Return 0 on success, < 0 on error.
 */
static int sfs_readdir(const char *path,
                       void *buf,
                       fuse_fill_dir_t filler,
                       off_t offset,
                       struct fuse_file_info *fi)
{
    (void)offset, (void)fi;
    log("readdir %s\n", path);

    (void)filler; /* Placeholder - use me */
    (void)buf; /* Placeholder - use me */

    if(strcmp(path, "/") == 0){
        log("rootpath");

        struct sfs_entry rootdir[SFS_ROOTDIR_NENTRIES];
        disk_read(rootdir, SFS_ROOTDIR_SIZE, SFS_ROOTDIR_OFF);

        for (unsigned int i=0; i < SFS_ROOTDIR_NENTRIES; i++){

            if(strlen(rootdir[i].filename) > 0){
                filler(buf, rootdir[i].filename, NULL, 0);
            }
        }
   }
   else {
        log("not rootpath");

        struct sfs_entry *entry = malloc(64);

        if (get_entry_rec(path, NULL, SFS_ROOTDIR_NENTRIES, 0, entry) > 0){
            return -ENOENT;
        }

        log("readdir back");

        struct sfs_entry temp[16];

        disk_read(temp, SFS_DIR_SIZE, SFS_DATA_OFF + entry->first_block * SFS_BLOCK_SIZE);
        
        for(unsigned int i=0; i < 16; i++){
            
            if(strlen(temp[i].filename) > 0){
                log("fill in entry %s", temp[i].filename);
                filler(buf, temp[i].filename, NULL, 0);
            } 
        }

    }

    return 0;
}


/*
 * Read contents of `path` into `buf` for  up to `size` bytes.
 * Note that `size` may be bigger than the file actually is.
 * Reading should start at offset `offset`; the OS will generally read your file
 * in chunks of 4K byte.
 * Returns the number of bytes read (writing into `buf`), or < 0 on error.
 */
static int sfs_read(const char *path,
                    char *buf,
                    size_t size,
                    off_t offset,
                    struct fuse_file_info *fi)
{
    (void)fi;
    log("read %s size=%zu offset=%ld\n", path, size, offset);

    (void)buf; /* Placeholder - use me */

    struct sfs_entry entry[16];

    if(get_entry_rec(path, NULL, SFS_ROOTDIR_NENTRIES, 0, entry) > 0){
        return -ENOENT;
    }
        
    blockidx_t blockID = entry[0].first_block;
    size_t remainingBytes;

    if(entry[0].size < size){
        remainingBytes = entry[0].size;
    }
    else {
        remainingBytes = size;
    }
        
    int bytesRead = 0;
    off_t currOffset = offset;
    
    // assert((blockID != SFS_BLOCKIDX_EMPTY) && (blockID != SFS_BLOCKIDX_END));

    while(currOffset >= 512){
        blockidx_t *buffer = malloc(2);
        disk_read(buffer, sizeof(blockidx_t), SFS_BLOCKTBL_OFF + blockID * 2);
        blockID = *buffer;
        currOffset -= 512;
    }

    while(remainingBytes > 0){

        currOffset = SFS_DATA_OFF + blockID * SFS_BLOCK_SIZE;

        if(remainingBytes < 512){
            disk_read(buf+bytesRead, remainingBytes, currOffset);
            bytesRead += remainingBytes;
            remainingBytes = 0;
            return bytesRead;
        }
        else if(remainingBytes >= 512){    
            disk_read(buf+bytesRead, SFS_BLOCK_SIZE, currOffset);
            bytesRead += 512;
            remainingBytes -= 512;
        }
        

        blockidx_t *buffer = malloc(2);
        disk_read(buffer, sizeof(blockidx_t), SFS_BLOCKTBL_OFF + blockID * 2);
        blockID = *buffer;

        log("block id: %x", blockID);
        // log("remaining bytes: %i", remainingBytes);

        currOffset = 0;
    }
    
    return bytesRead;    
}


/*
 * Create directory at `path`.
 * The `mode` argument describes the permissions, which you may ignore for this
 * assignment.
 * Returns 0 on success, < 0 on error.
 */
static int sfs_mkdir(const char *path,
                     mode_t mode)
{
    log("mkdir %s mode=%o\n", path, mode);

    if(strcmp(path, "/"))
        return -ENOSYS;


    return -ENOSYS;
}


/*
 * Remove directory at `path`.
 * Directories may only be removed if they are empty, otherwise this function
 * should return -ENOTEMPTY.
 * Returns 0 on success, < 0 on error.
 */
static int sfs_rmdir(const char *path)
{
    log("rmdir %s\n", path);

    return -ENOSYS;
}


/*
 * Remove file at `path`.
 * Can not be used to remove directories.
 * Returns 0 on success, < 0 on error.
 */
static int sfs_unlink(const char *path)
{
    log("unlink %s\n", path);

    return -ENOSYS;
}


/*
 * Create an empty file at `path`.
 * The `mode` argument describes the permissions, which you may ignore for this
 * assignment.
 * Returns 0 on success, < 0 on error.
 */
static int sfs_create(const char *path,
                      mode_t mode,
                      struct fuse_file_info *fi)
{
    (void)fi;
    log("create %s mode=%o\n", path, mode);

    return -ENOSYS;
}


/*
 * Shrink or grow the file at `path` to `size` bytes.
 * Excess bytes are thrown away, whereas any bytes added in the process should
 * be nil (\0).
 * Returns 0 on success, < 0 on error.
 */
static int sfs_truncate(const char *path, off_t size)
{
    log("truncate %s size=%ld\n", path, size);

    return -ENOSYS;
}


/*
 * Write contents of `buf` (of `size` bytes) to the file at `path`.
 * The file is grown if nessecary, and any bytes already present are overwritten
 * (whereas any other data is left intact). The `offset` argument specifies how
 * many bytes should be skipped in the file, after which `size` bytes from
 * buffer are written.
 * This means that the new file size will be max(old_size, offset + size).
 * Returns the number of bytes written, or < 0 on error.
 */
static int sfs_write(const char *path,
                     const char *buf,
                     size_t size,
                     off_t offset,
                     struct fuse_file_info *fi)
{
    (void)fi;
    log("write %s data='%.*s' size=%zu offset=%ld\n", path, (int)size, buf,
        size, offset);
    return -ENOSYS;   

}


/*
 * Move/rename the file at `path` to `newpath`.
 * Returns 0 on succes, < 0 on error.
 */
static int sfs_rename(const char *path,
                      const char *newpath)
{
    /* Implementing this function is optional, and not worth any points. */
    log("rename %s %s\n", path, newpath);

    return -ENOSYS;
}


static const struct fuse_operations sfs_oper = {
    .getattr    = sfs_getattr,
    .readdir    = sfs_readdir,
    .read       = sfs_read,
    .mkdir      = sfs_mkdir,
    .rmdir      = sfs_rmdir,
    .unlink     = sfs_unlink,
    .create     = sfs_create,
    .truncate   = sfs_truncate,
    .write      = sfs_write,
    .rename     = sfs_rename,
};


#define OPTION(t, p)                            \
    { t, offsetof(struct options, p), 1 }
#define LOPTION(s, l, p)                        \
    OPTION(s, p),                               \
    OPTION(l, p)
static const struct fuse_opt option_spec[] = {
    LOPTION("-i %s",    "--img=%s",     img),
    LOPTION("-b",       "--background", background),
    LOPTION("-v",       "--verbose",    verbose),
    LOPTION("-h",       "--help",       show_help),
    OPTION(             "--fuse-help",  show_fuse_help),
    FUSE_OPT_END
};

static void show_help(const char *progname)
{
    printf("usage: %s mountpoint [options]\n\n", progname);
    printf("By default this FUSE runs in the foreground, and will unmount on\n"
           "exit. If something goes wrong and FUSE does not exit cleanly, use\n"
           "the following command to unmount your mountpoint:\n"
           "  $ fusermount -u <mountpoint>\n\n");
    printf("common options (use --fuse-help for all options):\n"
           "    -i, --img=FILE      filename of SFS image to mount\n"
           "                        (default: \"%s\")\n"
           "    -b, --background    run fuse in background\n"
           "    -v, --verbose       print debug information\n"
           "    -h, --help          show this summarized help\n"
           "        --fuse-help     show full FUSE help\n"
           "\n", default_img);
}

int main(int argc, char **argv)
{
    struct fuse_args args = FUSE_ARGS_INIT(argc, argv);

    options.img = strdup(default_img);

    fuse_opt_parse(&args, &options, option_spec, NULL);

    if (options.show_help) {
        show_help(argv[0]);
        return 0;
    }

    if (options.show_fuse_help) {
        assert(fuse_opt_add_arg(&args, "--help") == 0);
        args.argv[0][0] = '\0';
    }

    if (!options.background)
        assert(fuse_opt_add_arg(&args, "-f") == 0);

    disk_open_image(options.img);

    return fuse_main(args.argc, args.argv, &sfs_oper, NULL);
}
