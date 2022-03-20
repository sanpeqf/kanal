/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright(c) 2021 Sanpe <sanpeqf@gmail.com>
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <err.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include "list.h"

static LIST_HEAD(func_list);

struct kanal_entry {
    struct list_head list;
    unsigned long long size;
    char *name;
};

#define list_to_kanal(ptr) \
    list_entry(ptr, struct kanal_entry, list)

static long kanal_cmp(struct list_head *nodea, struct list_head *nodeb, void *pdata)
{
    struct kanal_entry *kanala = list_to_kanal(nodea);
    struct kanal_entry *kanalb = list_to_kanal(nodeb);
    return kanala->size < kanalb->size? -1 : 1;
}

static int map_parser(const char *map)
{
    struct kanal_entry *curr, *prev = &(typeof(*prev)){ };
    const char *line, *next;
    char buff[512];

    for (line = map; line; line = next + 1) {
        unsigned long long addr;
        char *type, *name;

        next = strchr(line, '\n');
        if (!next) {
            fprintf(stderr, "EOF without '_end' record\n");
            return -ENFILE;
        }

        strncpy(buff, line, next - line);
        buff[next - line] = '\0';

        type = strchr(buff, ' ');
        if (!type)
            return -EINVAL;
        *type++ = '\0';

        name = strchr(type, ' ');
        if (!name)
            return -EINVAL;
        *name++ = '\0';

        curr = malloc(sizeof(*curr));
        if (!curr)
            return -ENOMEM;

        addr = strtoull(buff, NULL, 16);
        prev->size = addr - prev->size;
        curr->size = addr;
        curr->name = strdup(name);
        list_add(&func_list, &curr->list);

        if (!curr->name)
            return -ENOMEM;

        if (!strcmp(name, "_end")) {
            curr->size = 0;
            return 0;
        }

        prev = curr;
    }

    return -ENFILE;
}

static void *mmap_file(const char *file)
{
    struct stat stat;
    int fd, retval;
    void *data;

    if ((fd = open(file, O_RDONLY)) < 0)
        err(-1, "Cannot open file: %s", file);

    if ((retval = fstat(fd, &stat)) < 0)
        err(retval, "file fstat err");

    data = mmap(NULL, stat.st_size, PROT_READ, MAP_SHARED, fd, 0);
    if (data == MAP_FAILED)
        err(-1, "file mmap err");

    return data;
}

int main(int argc, char *argv[])
{
    struct kanal_entry *walk, *tmp;
    void *data;
    int retval;

    data = mmap_file(argv[1]);
    retval = map_parser(data);
    if (retval)
        return retval;

    list_sort(&func_list, kanal_cmp, NULL);
    list_for_each_entry_reverse_safe(walk, tmp, &func_list, list) {
        printf("0x%016llx %s\n", walk->size, walk->name);
        free(walk->name);
        free(walk);
    }

    return 0;
}
