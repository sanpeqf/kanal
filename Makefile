# SPDX-License-Identifier: GPL-2.0-or-later
flags = -std=gnu11 -Wall -Werror
heads = list.h
objs  = kanal.o sort.o

%.o:%.c $(heads)
	@ echo -e "  \e[32mCC\e[0m	" $@
	@ gcc -o $@ -c $< -g $(flags)

kanal: $(objs)
	@ echo -e "  \e[34mMKELF\e[0m	" $@
	@ gcc -o $@ $^ -g $(flags)

clean:
	@ rm -f $(objs) kanal
