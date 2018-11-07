TGT  = libfuse3.a
SRC  = $(wildcard lib/*.c)
OBJ  = $(SRC:lib/%.c=obj/%.o)
DEPS = $(OBJ:obj/%.o=obj/%.d)
CFLAGS = -g \
	 -Wall \
	 '-DFUSE_USE_VERSION=32' \
	 '-DFUSERMOUNT_DIR="/usr/local/bin"' \
	 -I./include \
	 -MMD

$(TGT): obj/obj-stamp $(OBJ)
	ar rcs obj/libfuse3.a $(OBJ)

obj/obj-stamp:
	mkdir -p obj
	touch $@

obj/%.o: lib/%.c
	$(CC) $(CFLAGS) -c $< -o $@
