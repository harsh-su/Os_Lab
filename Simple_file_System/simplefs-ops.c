#include "simplefs-ops.h"

extern struct filehandle_t file_handle_array[MAX_OPEN_FILES];

int simplefs_create(char *filename) {
    for (int i = 0; i < NUM_INODES; i++) {
        struct inode_t temp_inode;
        simplefs_readInode(i, &temp_inode);
        if (temp_inode.status == INODE_IN_USE && strcmp(temp_inode.name, filename) == 0) {
            return -1;  
        }
    }

    struct superblock_t superblock;
    simplefs_readSuperBlock(&superblock);

    int free_inode = -1;
    for (int i = 0; i < NUM_INODES; i++) {
        if (superblock.inode_freelist[i] == INODE_FREE) {
            free_inode = i;
            break;
        }
    }

    if (free_inode == -1) {
        return -1;  // No free inode available
    }

    // Allocate a free inode
    superblock.inode_freelist[free_inode] = INODE_IN_USE;
    simplefs_writeSuperBlock(&superblock);

    struct inode_t new_inode;
    new_inode.status = INODE_IN_USE;
    strcpy(new_inode.name, filename);
    new_inode.file_size = 0;
    for (int i = 0; i < MAX_FILE_SIZE; i++) {
        new_inode.direct_blocks[i] = -1;
    }
    // Write the new inode to disk
    simplefs_writeInode(free_inode, &new_inode);
    return free_inode;  // Return the inode number of the newly created file
}
void simplefs_delete(char *filename) {
    int inode_num = -1;

    // Find the inode number of the file by its name
    for (int i = 0; i < NUM_INODES; i++) {
        struct inode_t inode;
        simplefs_readInode(i, &inode);
        if (inode.status == INODE_IN_USE && strcmp(inode.name, filename) == 0) {
            inode_num = i;
            break;
        }
    }

    if (inode_num == -1) {
        return;
    }

    // Free data blocks associated with the file
    struct inode_t inode;
    simplefs_readInode(inode_num, &inode);
    for (int i = 0; i < MAX_FILE_SIZE; i++) {
        if (inode.direct_blocks[i] != -1) {
            simplefs_freeDataBlock(inode.direct_blocks[i]);
        }
    }

    // Free the inode
    simplefs_freeInode(inode_num);
}

int simplefs_open(char *filename) {
    // Find the inode number of the file by its name
    int inode_num = -1;
    for (int i = 0; i < NUM_INODES; i++) {
        struct inode_t inode;
        simplefs_readInode(i, &inode);
        if (inode.status == INODE_IN_USE && strcmp(inode.name, filename) == 0) {
            inode_num = i;
            break;
        }
    }

    if (inode_num == -1) {
        // File not found
        return -1;
    }

    // Allocate an unused file handle from the global file handle array
    int file_handle = -1;
    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        if (file_handle_array[i].inode_number == -1) {
            file_handle = i;
            file_handle_array[i].inode_number = inode_num;
            file_handle_array[i].offset = 0;
            break;
        }
    }

    return file_handle;
}

void simplefs_close(int file_handle) {
    // Check if the file handle is valid
    if (file_handle >= 0 && file_handle < MAX_OPEN_FILES) {
        // Mark the file handle as unused
        file_handle_array[file_handle].inode_number = -1;
        file_handle_array[file_handle].offset = 0;
    }
}

int simplefs_read(int file_handle, char *buf, int nbytes) {
    int inode_number = file_handle_array[file_handle].inode_number;
    int read_offset = file_handle_array[file_handle].offset;

    struct inode_t *file_inode = (struct inode_t *)malloc(sizeof(struct inode_t));
    simplefs_readInode(inode_number, file_inode);

    int file_size = file_inode->file_size;

    if (read_offset + nbytes > file_size) {
        // Invalid read operatio
        free(file_inode);
        return -1;
    }

    int block_number = read_offset / BLOCKSIZE;
    int offset_in_block = read_offset % BLOCKSIZE;
    int remaining_bytes = nbytes;

    while (remaining_bytes > 0) {
        char *blockdata = (char *)malloc(BLOCKSIZE);
        simplefs_readDataBlock(file_inode->direct_blocks[block_number], blockdata);

        int bytes_to_copy;
        if (remaining_bytes < BLOCKSIZE - offset_in_block){
            bytes_to_copy = remaining_bytes;
        }else {
            bytes_to_copy = BLOCKSIZE-offset_in_block;
        }

        memcpy(buf, blockdata + offset_in_block, bytes_to_copy);

        buf += bytes_to_copy;
        remaining_bytes -= bytes_to_copy;
        offset_in_block = 0;
        block_number++;

        free(blockdata);
    }

    free(file_inode);

    return 0;
}


int simplefs_write(int file_handle, char *buf, int nbytes) {
    int first_write_flag = 0;
    int write_offset = file_handle_array[file_handle].offset;
    int inode_number = file_handle_array[file_handle].inode_number;

    struct inode_t *file_inode = (struct inode_t *)malloc(sizeof(struct inode_t));
    simplefs_readInode(inode_number, file_inode);

    int file_size = file_inode->file_size;

    if (write_offset + nbytes > MAX_FILE_SIZE * BLOCKSIZE) {
        free(file_inode);
        return -1;
    }
    if (write_offset + nbytes > file_size) {
        file_size = write_offset + nbytes;
        file_inode->file_size = file_size;
    }

    int block_number = write_offset / BLOCKSIZE;
    int offset_in_block = write_offset % BLOCKSIZE;
    int available_to_write = (BLOCKSIZE - offset_in_block);
    int bytes_written = 0;

    while (nbytes > available_to_write) {
        char *blockdata = (char *)malloc(BLOCKSIZE);
            // Allocate a new data block
        if (file_inode->direct_blocks[block_number] == -1) {
            int new_block_number = simplefs_allocDataBlock();
            if (new_block_number < 0)
                return -1;
            file_inode->direct_blocks[block_number] = new_block_number;
            first_write_flag = 1;
        }

        if (!first_write_flag) {
            simplefs_readDataBlock(file_inode->direct_blocks[block_number], blockdata);
        }

        // Update data block with new content
        buf += bytes_written;
        memcpy(blockdata + offset_in_block, buf, available_to_write);
        simplefs_writeDataBlock(file_inode->direct_blocks[block_number], blockdata);

        first_write_flag = 0;
        bytes_written += available_to_write;
        nbytes -= available_to_write;
        offset_in_block = 0;
        block_number++;
        available_to_write = BLOCKSIZE - offset_in_block;

        free(blockdata);
    }

    char *blockdata = (char *)malloc(BLOCKSIZE);

    if (file_inode->direct_blocks[block_number] == -1) {
        // Allocate a new data block for the last write
        int new_block_number = simplefs_allocDataBlock();
        if (new_block_number < 0)
            return -1;
        file_inode->direct_blocks[block_number] = new_block_number;
        first_write_flag = 1;
    }

    if (!first_write_flag) {
        // Read existing data block if not the first write for the last block
        simplefs_readDataBlock(file_inode->direct_blocks[block_number], blockdata);
    }

    // Update the last data block with the remaining content
    buf += bytes_written;
    memcpy(blockdata + offset_in_block, buf, nbytes);
    simplefs_writeDataBlock(file_inode->direct_blocks[block_number], blockdata);

    free(blockdata);

    // Update the inode on disk
    simplefs_writeInode(inode_number, file_inode);
    free(file_inode);

    return 0;
}

int simplefs_seek(int file_handle, int nseek) {
    int write_offset = file_handle_array[file_handle].offset;
    int inode_number = file_handle_array[file_handle].inode_number;

    struct inode_t *file_inode = (struct inode_t *)malloc(sizeof(struct inode_t));
    simplefs_readInode(inode_number, file_inode);
    int file_size = file_inode->file_size;

    // Check if the seek operation is within the file boundaries
    if ((write_offset + nseek <= file_size) && (write_offset + nseek >= 0)) {
        write_offset += nseek;
        file_handle_array[file_handle].offset = write_offset;
    }

    free(file_inode);
    // Return the result of the seek operation
    return (write_offset + nseek <= file_size) && (write_offset + nseek >= 0);
}

