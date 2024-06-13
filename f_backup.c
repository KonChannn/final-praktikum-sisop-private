#include "kernel.h"
#include "std_lib.h"
#include "filesystem.h"

void fsInit() {
    struct map_fs map_fs_buf;
    int i = 0;

    readSector(&map_fs_buf, FS_MAP_SECTOR_NUMBER);
    for (i = 0; i < 16; i++) map_fs_buf.is_used[i] = true;
    for (i = 256; i < 512; i++) map_fs_buf.is_used[i] = true;
    writeSector(&map_fs_buf, FS_MAP_SECTOR_NUMBER);
}

// TODO: 2. Implement fsRead function
void fsRead(struct file_metadata* metadata, enum fs_return* status) {
    struct node_fs node_fs_buf;
    struct data_fs data_fs_buf;
    int i, j;
    bool found = false;

    // Read filesystem sectors into memory
    readSector(&node_fs_buf.nodes[0], FS_NODE_SECTOR_NUMBER);
    readSector(&data_fs_buf, FS_DATA_SECTOR_NUMBER);

    // Search for the node with the matching name and parent index
    for (i = 0; i < FS_MAX_NODE; i++) {
        if (strcmp(node_fs_buf.nodes[i].node_name, metadata->node_name) &&
            node_fs_buf.nodes[i].parent_index == metadata->parent_index) {
            found = true;
            break;
        }
    }

    if (!found) {
        *status = FS_R_NODE_NOT_FOUND;
        return;
    }

    // Check if the found node is a directory
    if (node_fs_buf.nodes[i].data_index == FS_NODE_D_DIR) {
        *status = FS_R_TYPE_IS_DIRECTORY;
        return;
    }

    // The found node is a file, read its data
    metadata->filesize = 0;
    for (j = 0; j < FS_MAX_SECTOR; j++) {
        if (data_fs_buf.datas[node_fs_buf.nodes[i].data_index].sectors[j] == 0x00) {
            break;
        }
        readSector(metadata->buffer + j * SECTOR_SIZE, data_fs_buf.datas[node_fs_buf.nodes[i].data_index].sectors[j]);
        metadata->filesize += SECTOR_SIZE;
    }

    *status = FS_SUCCESS;
}

// TODO: 3. Implement fsWrite function
void fsWrite(struct file_metadata* metadata, enum fs_return* status) {
    struct map_fs map_fs_buf;
    struct node_fs node_fs_buf;
    struct data_fs data_fs_buf;
    int i, j, k;
    int free_node_index = -1, free_data_index = -1;
    int free_block_count = 0;

    // Read filesystem sectors into memory
    readSector(&map_fs_buf, FS_MAP_SECTOR_NUMBER);
    readSector(&node_fs_buf.nodes[0], FS_NODE_SECTOR_NUMBER);
    readSector(&data_fs_buf, FS_DATA_SECTOR_NUMBER);

    // Search for a node with the same name and parent index
    for (i = 0; i < FS_MAX_NODE; i++) {
        if (strcmp(node_fs_buf.nodes[i].node_name, metadata->node_name) &&
            node_fs_buf.nodes[i].parent_index == metadata->parent_index) {
            *status = FS_W_NODE_ALREADY_EXISTS;
            return;
        }
    }

    // Find a free node
    for (i = 0; i < FS_MAX_NODE; i++) {
        if (node_fs_buf.nodes[i].node_name[0] == '\0') {
            free_node_index = i;
            break;
        }
    }
    if (free_node_index == -1) {
        *status = FS_W_NO_FREE_NODE;
        return;
    }

    // Find free data sectors
    for (i = 0; i < FS_MAX_DATA; i++) {
        if (data_fs_buf.datas[i].sectors[0] == 0x00) {
            free_data_index = i;
            break;
        }
    }
    if (free_data_index == -1) {
        *status = FS_W_NO_FREE_DATA;
        return;
    }

    // Check for enough free blocks in the map
    for (i = 0; i < SECTOR_SIZE; i++) {
        if (!map_fs_buf.is_used[i]) {
            free_block_count++;
        }
    }
    if (free_block_count < (metadata->filesize / SECTOR_SIZE)) {
        *status = FS_W_NOT_ENOUGH_SPACE;
        return;
    }

    // Set node metadata
    strcpy(node_fs_buf.nodes[free_node_index].node_name, metadata->node_name);
    node_fs_buf.nodes[free_node_index].parent_index = metadata->parent_index;
    node_fs_buf.nodes[free_node_index].data_index = free_data_index;

    // Initialize the data sectors of the new node
    for (j = 0; j < FS_MAX_SECTOR; j++) {
        data_fs_buf.datas[free_data_index].sectors[j] = 0x00;
    }

    // Write data to free blocks
    j = 0;
    for (i = 0; i < SECTOR_SIZE; i++) {
        if (!map_fs_buf.is_used[i]) {
            map_fs_buf.is_used[i] = true;
            data_fs_buf.datas[free_data_index].sectors[j] = i;
            writeSector(metadata->buffer + j * SECTOR_SIZE, i);
            j++;
            if (j * SECTOR_SIZE >= metadata->filesize) {
                break;
            }
        }
    }

    // Write updated filesystem back to disk
    writeSector(&map_fs_buf, FS_MAP_SECTOR_NUMBER);
    writeSector(&node_fs_buf.nodes[0], FS_NODE_SECTOR_NUMBER);
    writeSector(&data_fs_buf, FS_DATA_SECTOR_NUMBER);

    *status = FS_SUCCESS;
}

void listDirectoryContents(byte parent_index, char* buffer, int buffer_size, enum fs_return* status) {
    struct node_fs node_fs_buf;
    int i, offset = 0;

    // Read the node sector into memory
    readSector(&node_fs_buf, FS_NODE_SECTOR_NUMBER);

    for (i = 0; i < FS_MAX_NODE; i++) {
        if (node_fs_buf.nodes[i].parent_index == parent_index) {
            int name_length = strlen(node_fs_buf.nodes[i].node_name);
            if (offset + name_length + 1 >= buffer_size) {
                *status = FS_W_NOT_ENOUGH_SPACE;
                return;
            }
            strcpy(buffer + offset, node_fs_buf.nodes[i].node_name);
            offset += name_length;
            buffer[offset++] = '\n';
        }
    }

    buffer[offset] = '\0';
    *status = FS_SUCCESS;
}

void findDirectory(byte parent_index, char* name, byte* found_index, enum fs_return* status) {
    struct node_fs node_fs_buf;
    int i;

    // Read the node sector into memory
    readSector(&node_fs_buf, FS_NODE_SECTOR_NUMBER);

    for (i = 0; i < FS_MAX_NODE; i++) {
        if (node_fs_buf.nodes[i].parent_index == parent_index && strcmp(node_fs_buf.nodes[i].node_name, name)) {
            *found_index = i;
            *status = FS_SUCCESS;
            return;
        }
    }

    *status = FS_R_NODE_NOT_FOUND;
}