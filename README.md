# Abstract for Assignment 3: Filesystem

This is a filesystem driver, SFS, which emulates core functionalities of a filesystem, based on the FAT structure. The project facilitates the management of files and directories within a constrained environment, offering operations for creation, navigation, reading, and modification of files.

Key features included:

- **Filesystem Structure**: SFS supports a root directory and subdirectories, enabling hierarchical organization, while adhering to limitations such as a maximum size of 8 MB and a restricted number of entries per directory.

- **Driver Implementation**: Utilizing the FUSE framework, the SFS driver interacts seamlessly with the Linux kernel, allowing users to mount the filesystem as if it were a standard disk partition.

- **Data Management**: The filesystem architecture incorporates a block table for efficient data retrieval, linking file segments and maintaining the structure for file allocation.

- **Entry Format**: Each directory entry is defined by a fixed structure, encompassing filename, block index, and size, ensuring uniform access across the filesystem.

This assignment provided valuable insight in filesystem design and implementation, reinforcing critical concepts in operating systems.
