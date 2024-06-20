#include "shell.h"
#include "kernel.h"
#include "std_lib.h"
#include "filesystem.h"


void shell() {
  char buf[64];
  char cmd[64];
  char arg[2][64];

  struct node_fs node_fs_buf;
  struct node_item* node;

  byte cwd = FS_NODE_P_ROOT;

  while (true) {
    printString("MengOS:");
    printCWD(cwd);
    printString("$ ");
    readString(buf);
    parseCommand(buf, cmd, arg);

    // printString(buf);
    // printString("\n");
    // printString(cmd);  
    // printString("\n");
    // printString(arg);
    // printString("\n");
    // printString(arg[1]);
    // printString("\n");


    if (strcmp(cmd, "cd")) cd(&cwd, arg[0]);
    else if (strcmp(cmd, "ls")) ls(cwd, arg[0]);
    else if (strcmp(cmd, "mv")) mv(cwd, arg[0], arg[1]);
    else if (strcmp(cmd, "cp")) cp(cwd, arg[0], arg[1]);
    else if (strcmp(cmd, "cat")) cat(cwd, arg[0]);
    else if (strcmp(cmd, "mkdir")) mkdir(cwd, arg[0]);
    else if (strcmp(cmd, "clear")) clearScreen();
    else if (strcmp(cmd, "pwd")) {
      printCWD(cwd);
      printString("\n");
    }
    else printString("Invalid command\n");
  }
}


void printCWD(byte cwd) {
    struct node_fs node_fs_buf;
    char path[256];  // Buffer to store the full path
    char temp[64];        // Temporary buffer for each directory name
    int length;       // Current length of the path
    int temp_length;
    int i;
    int j;

    length = 0;

    // Read the node sectors into memory
    readSector(&(node_fs_buf.nodes[0]), FS_NODE_SECTOR_NUMBER);
    readSector(&(node_fs_buf.nodes[32]), FS_NODE_SECTOR_NUMBER + 1);

    // Traverse from the current node to the root
    while (cwd != FS_NODE_P_ROOT) {
        // Copy the current node name to the temporary buffer
        for (i = 0; i < 64 && node_fs_buf.nodes[cwd].node_name[i] != '\0'; i++) {
            temp[i] = node_fs_buf.nodes[cwd].node_name[i];
        }
        temp[i] = '\0'; // Null-terminate the string

        // Prepend the current directory name and '/' to the path
        

        temp_length = i + 1; // Directory name length + '/'
        length += temp_length;
        if (length >= 256) {
            // Prevent buffer overflow
            printString("Path length exceeds buffer size.\n");
            return;
        }

        // Move existing path to make space for new directory
        for (j = length - 1; j >= temp_length; j--) {
            path[j] = path[j - temp_length];
        }

        // Add the new directory name and '/'
        path[0] = '/';
        for (i = 0; i < temp_length - 1; i++) {
            path[i + 1] = temp[i];
        }

        // Move to the parent directory
        cwd = node_fs_buf.nodes[cwd].parent_index;
    }

    // Add root '/' if path is empty
    if (length == 0) {
        path[length++] = '/';
        path[length] = '\0';
    }

    // Print the constructed path
    printString(path);
}


void parseCommand(char* buf, char* cmd, char arg[2][64]) {
    int i = 0, j = 0, argIndex = 0;
    bool inToken = false;

    // Initialize the command and arguments to empty strings
    cmd[0] = '\0';
    arg[0][0] = '\0';
    arg[1][0] = '\0';

    // Skip leading spaces
    while (buf[i] == ' ') i++;

    // Parse the command
    while (buf[i] != ' ' && buf[i] != '\0') {
        cmd[j++] = buf[i++];
    }
    cmd[j] = '\0';

    // Skip spaces between command and arguments
    while (buf[i] == ' ') i++;

    // Parse arguments
    while (buf[i] != '\0' && argIndex < 2) {
        j = 0;
        while (buf[i] != ' ' && buf[i] != '\0') {
            arg[argIndex][j++] = buf[i++];
        }
        arg[argIndex][j] = '\0';
        argIndex++;

        // Skip spaces between arguments
        while (buf[i] == ' ') i++;
    }
}


void cd(byte* cwd, char* dirname) {
    struct node_fs node_fs_buf;
    struct node_item* node;
    int i;
    byte dir_index;

    // printString(dirname);
    // printString("\n");
    // printString(cwd);
    // printString("\n");

    // 1. Read the filesystem from disk to memory.
    readSector(&(node_fs_buf.nodes[0]), FS_NODE_SECTOR_NUMBER);
    readSector(&(node_fs_buf.nodes[32]), FS_NODE_SECTOR_NUMBER);

    // if (strcmp(cmd, "cd")) cd(&cwd, arg[0]);

    if (strcmp(dirname, "/")) {
        // Move to the root directory
        *cwd = FS_NODE_P_ROOT;
        printString("Root Directory\n");
    } else if (strcmp(dirname, "..")) {
        // Move to the parent directory if it's not already at the root
        //printCWD(node_fs_buf.nodes[*cwd].parent_index);
        if (*cwd != FS_NODE_P_ROOT) {
            *cwd = node_fs_buf.nodes[*cwd].parent_index;
            // printCWD(*cwd);
            // printString("\n");
            // printCWD(cwd);
            // printString("\n");
            // strcpy(cwd, *cwd);
            // printCWD(cwd);
            // printString("\n");
            return;
        }
    } else {
        // Find the directory with the given name under the current working directory
        //printCWD(node_fs_buf.nodes[*cwd].parent_index);
        //printString("\n");
        dir_index = FS_NODE_P_ROOT;
        for (i = 0; i < FS_MAX_NODE; i++) {
            node = &node_fs_buf.nodes[i];
            if (node->parent_index == *cwd && node->data_index == FS_NODE_D_DIR && strcmp(node->node_name, dirname)) {
                *cwd = i;
                // printCWD(*cwd);
                // printString("\n");
                return;
            }
        }

        printString("Directory not found\n");
    }
}

void ls(byte cwd, char* dirname) {
    struct node_fs node_fs_buf;
    // struct node_item* node;
    int i;

    // 1. Read the filesystem from disk to memory.
    readSector(&(node_fs_buf.nodes[0]), FS_NODE_SECTOR_NUMBER);
    readSector(&(node_fs_buf.nodes[32]), FS_NODE_SECTOR_NUMBER + 1);

    if (dirname == '\0' || strcmp(dirname, "") || strcmp(dirname, ".")) {
        // List the contents of the current working directory
        // printString("Contents of current working directory:\n");
        for (i = 0; i < FS_MAX_NODE; i++) {
            if (node_fs_buf.nodes[i].parent_index == cwd) {
                if (node_fs_buf.nodes[i].data_index == FS_NODE_D_DIR) {
                    printString("[DIR] ");
                } else {
                    printString("[FILE] ");
                }
                printString(node_fs_buf.nodes[i].node_name);
                printString("\n");
            }
        }
        return;

    } else {
        // Find the directory with the given name under the current working directory
        byte dir_index = FS_NODE_P_ROOT;
        for (i = 0; i < FS_MAX_NODE; i++) {
            if (strcmp(node_fs_buf.nodes[i].node_name, dirname)) {
                if (node_fs_buf.nodes[i].data_index == FS_NODE_D_DIR) {
                    dir_index = i;
                    // List the contents of the found directory
                    for (i = 0; i < FS_MAX_NODE; i++) {
                        if (node_fs_buf.nodes[i].parent_index == dir_index) {
                            if (node_fs_buf.nodes[i].data_index == FS_NODE_D_DIR) {
                                printString("[DIR] ");
                            } else {
                                printString("[FILE] ");
                            }
                            printString(node_fs_buf.nodes[i].node_name);
                            printString("\n");
                        }
                    }
                    return;
                } else {
                    printString(dirname);
                    printString(" is not a directory\n");
                    return;
                }
            }
        }


        if (dir_index == FS_NODE_P_ROOT) {
            printString("Directory not found\n");
            return;
        }

        
    }
}

void mv(byte cwd, char* src, char* dst) {}


void cp(byte cwd, char* src, char* dst) {}


void cat(byte cwd, char* filename) {}


void mkdir(byte cwd, char* dirname) {
    struct node_fs node_fs_buf;
    struct node_item* node;
    byte free_node_index;
    int i;

    // Read the filesystem from disk to memory.
    readSector(&(node_fs_buf.nodes[0]), FS_NODE_SECTOR_NUMBER);
    readSector(&(node_fs_buf.nodes[32]), FS_NODE_SECTOR_NUMBER + 1);

    // Check if directory already exists
    for (i = 1; i < FS_MAX_NODE; i++) {
        node = &node_fs_buf.nodes[i];
        if (node->parent_index == cwd && strcmp(node->node_name, dirname)) {
            printString("Directory already exists\n");
            return;
        }
    }

    // Find a free node index to create the new directory
    free_node_index = FS_MAX_NODE;  // Assume no free node found initially
    for (i = 1; i < FS_MAX_NODE; i++) {
        if (node_fs_buf.nodes[i].parent_index == 0 && node_fs_buf.nodes[i].data_index == 0) {
            free_node_index = i;
            break;
        }
    }

    if (free_node_index == FS_MAX_NODE) {
        printString("No free node available\n");
        return;
    }

    // Initialize the new directory node
    node_fs_buf.nodes[free_node_index].parent_index = cwd;
    node_fs_buf.nodes[free_node_index].data_index = FS_NODE_D_DIR;
    strcpy(node_fs_buf.nodes[free_node_index].node_name, dirname);
    node_fs_buf.nodes[free_node_index].node_name[MAX_FILENAME - 1] = '\0';  // Ensure null-termination

    // Write back the modified node_fs_buf to disk
    writeSector(&(node_fs_buf.nodes[0]), FS_NODE_SECTOR_NUMBER);
    writeSector(&(node_fs_buf.nodes[32]), FS_NODE_SECTOR_NUMBER + 1);

    printString("Directory created successfully\n");
}
