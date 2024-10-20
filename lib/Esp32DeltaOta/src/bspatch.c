/*
 * Part of Esp32BleControl a firmware to allow ESP32 remote control over BLE.
 * Copyright (C) 2024  Simon Fischer
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <limits.h>

#ifdef ESP_PLATFORM
#include <sdkconfig.h>
#endif

#ifndef CONFIG_BSDIFF_BSPATCH_BUF_SIZE
// #define CONFIG_BSDIFF_BSPATCH_BUF_SIZE (8 * 1024)
#define CONFIG_BSDIFF_BSPATCH_BUF_SIZE (256)
#endif


#include "bspatch.h"

/* CONFIG_BSDIFF_BSPATCH_BUF_SIZE can't be smaller than 8 */
#if 7 >= CONFIG_BSDIFF_BSPATCH_BUF_SIZE
#error "Error, CONFIG_BSDIFF_BSPATCH_BUF_SIZE can't be smaller than 8"
#endif

#define BUF_SIZE CONFIG_BSDIFF_BSPATCH_BUF_SIZE
#define ERROR_BSPATCH (-1)

#define RETURN_IF_NON_ZERO(expr)        \
    do                                  \
    {                                   \
        const int ret = (expr);         \
        if (ret) {                      \
            return ret;                 \
        }                               \
    } while(0)

#define min(A, B) ((A) < (B) ? (A) : (B))

static int64_t offtin(uint8_t *buf)
{
	int64_t y;

	y=buf[7]&0x7F;
	y=y*256;y+=buf[6];
	y=y*256;y+=buf[5];
	y=y*256;y+=buf[4];
	y=y*256;y+=buf[3];
	y=y*256;y+=buf[2];
	y=y*256;y+=buf[1];
	y=y*256;y+=buf[0];

	if(buf[7]&0x80) y=-y;

	return y;
}

/*
 * Returns 0 on success
 * Returns -1 on error in patching logic
 * Returns any non-zero return code from stream read() and write() functions (which imply error)
 */
int bspatch(struct bspatch_stream_i *old, int64_t oldsize, struct bspatch_stream_n *new, int64_t newsize, struct bspatch_stream* stream)
{
	uint8_t buf[BUF_SIZE];
	int64_t oldpos,newpos;
	int64_t ctrl[3];
	int64_t i,k;
	int64_t towrite;
	const int64_t half_len = BUF_SIZE / 2;

	oldpos=0;newpos=0;
	while(newpos<newsize) {
		/* Read control data */
		for(i=0;i<=2;i++) {
			RETURN_IF_NON_ZERO(stream->read(stream, buf, 8));
			ctrl[i]=offtin(buf);
		}

		/* Sanity-check */
		if (ctrl[0]<0 || ctrl[0]>INT_MAX ||
			ctrl[1]<0 || ctrl[1]>INT_MAX ||
			newpos+ctrl[0]>newsize)
			return -100;

		/* Read diff string and add old data on the fly */
		i = ctrl[0];
		while (i) {
			towrite = min(i, half_len);
			RETURN_IF_NON_ZERO(stream->read(stream, &buf[half_len], towrite));
			RETURN_IF_NON_ZERO(old->read(old, buf, oldpos + (ctrl[0] - i), towrite));

			for(k=0;k<towrite;k++)
				buf[k + half_len] += buf[k];

			RETURN_IF_NON_ZERO(new->write(new, &buf[half_len], towrite));
			i -= towrite;
		}

		/* Adjust pointers */
		newpos+=ctrl[0];
		oldpos+=ctrl[0];

		/* Sanity-check */
		if(newpos+ctrl[1]>newsize)
			return -101;

		/* Read extra string and copy over to new on the fly*/
		i = ctrl[1];
		while (i) {
			towrite = min(i, BUF_SIZE);
			RETURN_IF_NON_ZERO(stream->read(stream, buf, towrite));
			RETURN_IF_NON_ZERO(new->write(new, buf, towrite));
			i -= towrite;
		}

		/* Adjust pointers */
		newpos+=ctrl[1];
		oldpos+=ctrl[2];
	};

	return 0;
}

#if defined(BSPATCH_EXECUTABLE)

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <err.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

static int __read(const struct bspatch_stream* stream, void* buffer, int length)
{
	if (fread(buffer, 1, length, (FILE*)stream->opaque) != length) {
		return -1;
	}
	return 0;
}

static int old_read(const struct bspatch_stream_i* stream, void* buffer, int pos, int length) {
	uint8_t* old;
	old = (uint8_t*)stream->opaque;
	memcpy(buffer, old + pos, length);
	return 0;
}

struct NewCtx {
	uint8_t* new;
	int pos_write;
};

static int new_write(const struct bspatch_stream_n* stream, const void *buffer, int length) {
	struct NewCtx* new;
	new = (struct NewCtx*)stream->opaque;
	memcpy(new->new + new->pos_write, buffer, length);
	new->pos_write += length;
	return 0;
}

int main(int argc,char * argv[])
{
	FILE * f;
	int fd;
	uint8_t *old, *new;
	int64_t oldsize, newsize;
	struct bspatch_stream stream;
	struct bspatch_stream_i oldstream;
	struct bspatch_stream_n newstream;
	struct stat sb;

	if(argc!=5) errx(1,"usage: %s oldfile newfile newsize patchfile\n",argv[0]);

	newsize = atoi(argv[3]);

	/* Open patch file */
	if ((f = fopen(argv[4], "r")) == NULL)
		err(1, "fopen(%s)", argv[4]);

	/* Close patch file and re-open it at the right places */
	if(((fd=open(argv[1],O_RDONLY,0))<0) ||
		((oldsize=lseek(fd,0,SEEK_END))==-1) ||
		((old=malloc(oldsize+1))==NULL) ||
		(lseek(fd,0,SEEK_SET)!=0) ||
		(read(fd,old,oldsize)!=oldsize) ||
		(fstat(fd, &sb)) ||
		(close(fd)==-1)) err(1,"%s",argv[1]);
	if((new=malloc(newsize+1))==NULL) err(1,NULL);

	stream.read = __read;
	oldstream.read = old_read;
	newstream.write = new_write;
	stream.opaque = f;
	oldstream.opaque = old;
	struct NewCtx ctx = { .pos_write = 0, .new = new };
	newstream.opaque = &ctx;
	if (bspatch(&oldstream, oldsize, &newstream, newsize, &stream))
		errx(1, "bspatch");

	/* Clean up */
	fclose(f);

	/* Write the new file */
	if(((fd=open(argv[2],O_CREAT|O_TRUNC|O_WRONLY,sb.st_mode))<0) ||
		(write(fd,new,newsize)!=newsize) || (close(fd)==-1))
		err(1,"%s",argv[2]);

	free(new);
	free(old);

	return 0;
}

#endif
