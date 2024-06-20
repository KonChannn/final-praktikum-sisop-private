// TODO: 7. Implement ls function
void ls(byte cwd, char* dirname) {
    struct node_fs node_fs_buf;
    int i;
    bool is_current_dir = false;

    // Read the filesystem nodes into memory
    readSector(&(node_fs_buf.nodes[0]), FS_NODE_SECTOR_NUMBER);
    readSector(&(node_fs_buf.nodes[32]), FS_NODE_SECTOR_NUMBER + 1);

    // If dirname is "." or empty, list the contents of the current directory
    if (strcmp(dirname, ".") || strlen(dirname) == 0) {
        is_current_dir = true;
    }

    // If it's the current directory, list the contents of cwd
    if (is_current_dir) {
        for (i = 0; i < FS_MAX_NODE; i++) {
            if (node_fs_buf.nodes[i].parent_index == cwd) {
                printString(node_fs_buf.nodes[i].node_name);
                if (node_fs_buf.nodes[i].data_index == FS_NODE_D_DIR) {
                    printString("/\n");
                } else {
                    printString("\n");
                }
            }
        }
        return;
    }

    // Search for the directory in the current working directory
    for (i = 0; i < FS_MAX_NODE; i++) {
        if (node_fs_buf.nodes[i].parent_index == cwd &&
            strcmp(node_fs_buf.nodes[i].node_name, dirname)) {
            if (node_fs_buf.nodes[i].data_index == FS_NODE_D_DIR) {
                // List the contents of the found directory
                byte dir_index = i;
                for (i = 0; i < FS_MAX_NODE; i++) {
                    if (node_fs_buf.nodes[i].parent_index == dir_index) {
                        printString(node_fs_buf.nodes[i].node_name);
                        if (node_fs_buf.nodes[i].data_index == FS_NODE_D_DIR) {
                            printString("/\n");
                        } else {
                            printString("\n");
                        }
                    }
                }
                return;
            } else {
                printString("ls: not a directory\n");
                return;
            }
        }
    }

    // If the directory is not found
    printString("ls: no such directory\n");
}