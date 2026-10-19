# Shredder

A lightweight, high-performance, and recursive file/directory shredder written in POSIX C. Designed to safely destroy sensitive data by overwriting underlying memory sectors before unlinking, making data recovery virtually impossible.

## Features

- **Multi-Pass Overwrite Protocol:** Overwrites targeted files across multiple passes (Pseudo/Real Random data from `/dev/urandom` and zero-fill `0x00`) to guarantee permanent destruction.
- **Cache Synchronization (`fsync`):** Forces kernel memory buffers to flush directly to physical storage media, bypassing standard OS write caching.
- **Recursive Directory Destruction:** Recursively traverses directory structures, shredding all child files before safely unlinking directories.
- **File Truncation:** Reduces inode size to zero before unlinking from the filesystem.
- **Zero External Dependencies:** Built purely using standard C and POSIX system calls.

## How It Works

1. **Inspection:** Checks target permissions and identifies whether the path is a file or directory.
2. **Overwriting:** Loops over the target file's exact byte size in chunks (`BUFFER_SIZE = 4096`), executing multi-pass overwrite sequences.
3. **Flushing:** Invokes `fsync()` after each pass to enforce physical hardware writes.
4. **Truncation & Unlinking:** Truncates file length to 0 bytes and unlinks (`unlink`/`rmdir`) it from the filesystem directory entry.

## Compilation

Compile with optimization flags using GCC or Clang:

```bash
gcc -O2 shredder.c -o shredder
