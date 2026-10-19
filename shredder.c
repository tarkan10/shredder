#define _XOPEN_SOURCE 500
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>
#include <dirent.h>

#define PASSES 3
#define BUFFER_SIZE 4096

void force_flush(int fd) {
    fsync(fd);
}

void generate_random_bytes(unsigned char *buf, size_t size) {
    int urandom_fd = open("/dev/urandom", O_RDONLY);
    if (urandom_fd >= 0) {
        if (read(urandom_fd, buf, size) < 0) {
            for (size_t i = 0; i < size; i++) {
                buf[i] = (unsigned char)(rand() % 256);
            }
        }
        close(urandom_fd);
    } else {
        for (size_t i = 0; i < size; i++) {
            buf[i] = (unsigned char)(rand() % 256);
        }
    }
}

int shred_file(const char *filepath) {
    struct stat st;
    if (stat(filepath, &st) != 0) {
        perror("[-] Failed to retrieve file status");
        return -1;
    }

    if (!S_ISREG(st.st_mode)) {
        fprintf(stderr, "[-] Error: '%s' is not a regular file.\n", filepath);
        return -1;
    }

    off_t file_size = st.st_size;
    printf("[*] Shredding File: %s (%ld bytes)\n", filepath, file_size);

    int fd = open(filepath, O_WRONLY);
    if (fd < 0) {
        perror("[-] Failed to open file for writing");
        return -1;
    }

    unsigned char buffer[BUFFER_SIZE];

    for (int pass = 1; pass <= PASSES; pass++) {
        printf("[+] Pass %d/%d... ", pass, PASSES);
        fflush(stdout);

        lseek(fd, 0, SEEK_SET);
        off_t bytes_written = 0;

        while (bytes_written < file_size) {
            size_t to_write = (file_size - bytes_written > BUFFER_SIZE) ? BUFFER_SIZE : (file_size - bytes_written);

            if (pass == PASSES) {
                memset(buffer, 0x00, to_write);
            } else {
                generate_random_bytes(buffer, to_write);
            }

            ssize_t written = write(fd, buffer, to_write);
            if (written <= 0) {
                perror("\n[-] Write error occurred");
                close(fd);
                return -1;
            }
            bytes_written += written;
        }

        force_flush(fd);
        printf("DONE.\n");
    }

    close(fd);

    if (truncate(filepath, 0) != 0) {
        perror("[!] Warning: Truncate failed");
    }

    if (unlink(filepath) == 0) {
        printf("[+] Destroyed: %s\n", filepath);
        return 0;
    } else {
        perror("[-] Unlink error");
        return -1;
    }
}

int shred_target(const char *path);

int shred_directory(const char *dirpath) {
    DIR *dir = opendir(dirpath);
    if (!dir) {
        perror("[-] Failed to open directory");
        return -1;
    }

    struct dirent *entry;
    char fullpath[1024];

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        snprintf(fullpath, sizeof(fullpath), "%s/%s", dirpath, entry->d_name);
        shred_target(fullpath);
    }

    closedir(dir);

    if (rmdir(dirpath) == 0) {
        printf("[+] Directory removed: %s\n", dirpath);
        return 0;
    } else {
        perror("[-] Failed to remove directory");
        return -1;
    }
}

int shred_target(const char *path) {
    struct stat st;
    if (lstat(path, &st) != 0) {
        perror("[-] Failed to stat target");
        return -1;
    }

    if (S_ISDIR(st.st_mode)) {
        return shred_directory(path);
    } else if (S_ISREG(st.st_mode)) {
        return shred_file(path);
    } else {
        fprintf(stderr, "[-] Skipping non-regular/non-directory target: %s\n", path);
        return -1;
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <file_or_directory_1> [target_2 ...]\n", argv[0]);
        return 1;
    }

    srand((unsigned int)time(NULL));

    for (int i = 1; i < argc; i++) {
        shred_target(argv[i]);
    }

    return 0;
}
