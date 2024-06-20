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


void fsRead(struct file_metadata* metadata, enum fs_return* status) {
  struct node_fs node_fs_buf;
  struct data_fs data_fs_buf;

  int i, j;
  int node_found = 0;
  struct node_item* node = 0;
  
  readSector(&data_fs_buf, FS_DATA_SECTOR_NUMBER);
  readSector(&(node_fs_buf.nodes[0]), FS_NODE_SECTOR_NUMBER);
  readSector(&(node_fs_buf.nodes[32]), FS_NODE_SECTOR_NUMBER);

    for (i = 0; i < FS_MAX_NODE; i++) {
        if (strcmp(node_fs_buf.nodes[i].node_name, metadata->node_name) == 0 && node_fs_buf.nodes[i].parent_index == metadata->parent_index) {
            node = &node_fs_buf.nodes[i];
            node_found = 1;
            break;
        }
    }

    if (!node_found) {
        *status = FS_R_NODE_NOT_FOUND;
        return;
    }

    // Check if the found node is a directory
    if (node_fs_buf.nodes[i].data_index == FS_NODE_D_DIR) {
        *status = FS_R_TYPE_IS_DIRECTORY;
        return;
    }

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

void fsWrite(struct file_metadata* metadata, enum fs_return* status) {
  struct map_fs map_fs_buf;
  struct node_fs node_fs_buf;
  struct data_fs data_fs_buf;

  int i, j;
  int empty_node_index;
  int empty_data_index;
  int empty_blocks;

  // 1. Read the filesystem from disk to memory.
  readSector(&data_fs_buf, FS_DATA_SECTOR_NUMBER);
  readSector(&(node_fs_buf.nodes[0]), FS_NODE_SECTOR_NUMBER);
  readSector(&(node_fs_buf.nodes[32]), FS_NODE_SECTOR_NUMBER);


  // 2. Iterate through each node item to find a node that has a name equal to metadata->node_name and a parent index equal to metadata->parent_index. If the searched node is found, then set status with FS_R_NODE_ALREADY_EXISTS and exit.
  for (i = 0; i < FS_MAX_NODE; i++) {
      if (strcmp(node_fs_buf.nodes[i].node_name, metadata->node_name) == 0 && node_fs_buf.nodes[i].parent_index == metadata->parent_index) {
          *status = FS_W_NODE_ALREADY_EXISTS;
          return;
      }
  }
  
  // 3. Next, search for an empty node (the node name is an empty string) and store its index. If an empty node is not found, then set status with FS_W_NO_FREE_NODE and exit.
  empty_node_index = -1;
  for (i = 0; i < FS_MAX_NODE; i++) {
      if (node_fs_buf.nodes[i].node_name[0] == '\0') {
          empty_node_index = i;
          break;
      }
  }
  if (empty_node_index == -1) {
      *status = FS_W_NO_FREE_NODE;
      return;
  }

  // 4. Iterate through each data item to find the empty data (the data sector address is 0x00) and store the index. If empty data is not found, then set status with FS_W_NO_FREE_DATA and exit.
  empty_data_index = -1;
  for (i = 0; i < FS_MAX_DATA; i++) {
      if (data_fs_buf.datas[i].sectors == 0x00) {
          empty_data_index = i;
          break;
      }
  }
  if (empty_data_index == -1) {
      *status = FS_W_NO_FREE_DATA;
      return;
  }

  // 5. Iterate through each map item and count the empty blocks (block status is 0x00 or false). If the empty blocks are less than metadata->filesize/SECTOR_SIZE, then set status with FS_W_NOT_ENOUGH_SPACE and exit.
  empty_blocks = 0;
  for (i = 0; i < SECTOR_SIZE; i++) {
      if (map_fs_buf.is_used[i] == false) {
          empty_blocks++;
      }
  }
  if (empty_blocks < metadata->filesize / SECTOR_SIZE) {
      *status = FS_W_NOT_ENOUGH_SPACE;
      return;
  }

  // 6. Set the name of the found node with metadata->node_name, 
  // parent index with metadata->parent_index, and data index with the empty data 
  // index.
  strcpy(node_fs_buf.nodes[empty_node_index].node_name, metadata->node_name);
  node_fs_buf.nodes[empty_node_index].parent_index = metadata->parent_index;
  node_fs_buf.nodes[empty_node_index].data_index = empty_data_index;

  // 7. Write the data in the following way.
  // Create a counter variable that will be used to count the number of sectors that have been written (will be called j).
  // Iterate i from 0 to SECTOR_SIZE.
  // If the map item at the i-th index is 0x00, then write index i into the j-th sector data item and write the data from the buffer into the i-th sector.
  // Writing can use the writeSector function of metadata->buffer + i * SECTOR_SIZE.
  // Add 1 to j.
  for (j = 0; j < FS_MAX_SECTOR; j++) {
        data_fs_buf.datas[empty_data_index].sectors[j] = 0x00;
    }

  j = 0;
  for (i = 0; i < SECTOR_SIZE; i++) {
    if (map_fs_buf.is_used[i] == false) {
      data_fs_buf.datas[empty_data_index].sectors[j] = i; // Store the sector index in the data sector
      writeSector(metadata->buffer + j * SECTOR_SIZE, i); // Write the data from the buffer to the sector
      // map_fs_buf.is_used[i] = true; // Mark the sector as used
      j++;
      if (j * SECTOR_SIZE >= metadata->filesize) {
                break;
            }
    }
  }

    // 8. Write the changed filesystem back to disk.
    writeSector(&map_fs_buf, FS_MAP_SECTOR_NUMBER);
    writeSector(&node_fs_buf.nodes[0], FS_NODE_SECTOR_NUMBER);
    writeSector(&data_fs_buf, FS_DATA_SECTOR_NUMBER);

    // 9. Set status with FS_W_SUCCESS.
    *status = FS_SUCCESS;

}
