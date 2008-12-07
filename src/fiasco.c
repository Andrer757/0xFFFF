/*
 *  0xFFFF - Open Free Fiasco Firmware Flasher
 *  Copyright (C) 2007  pancake <pancake@youterm.com>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include "main.h"
#include "hash.h"

int (*fiasco_callback)(struct header_t *header) = NULL;

int openfiasco(char *name)
{
	struct header_t header;
	unsigned char buf[128];
	unsigned char data[128];
	unsigned char *pdata;
	unsigned int namelen;
	off_t off, here;
	int i,j;
	header.fd = open(name, O_RDONLY);

	if (header.fd == -1) {
		fprintf(stderr, "Cannot open %s\n", name);
		return 1;
	}

	/* read header */
	read(header.fd, buf, 5);
	if (buf[0] != 0xb4) {
		printf("Invalid header\n");
		return close(header.fd);
	}
	memcpy(&namelen,buf+1,4);
	namelen = ntohl(namelen);
	if (namelen>128) {
		printf("Stupid length at header. Is this a joke?\n");
		return close(header.fd);
	}
	memset(buf,'\0', 128);
	read(header.fd, buf, namelen);

	printf("Fiasco version: %2d\n", buf[3]);
	strcpy(header.fwname, (char *)buf+6);
	for(i=6;i<namelen;i+=strlen((char *)(buf+i))+1)
		printf("Name: %s\n", buf+i);

	/* walk the tree */
	while(1) {
		here = lseek(header.fd, 0, SEEK_CUR);
		if (read(header.fd, buf, 9)<9)
			break;
		if (buf[0] != 0x54) { // 'T'
			/* skipping some bytes */
			lseek(header.fd, -9, SEEK_CUR);
			for(i=0;buf[0]!=0x54;i++) {
				j = read(header.fd, buf, 1);
				if (j == -1) {
					printf("Cannot find next FIASCO header\n");
					return close(header.fd);
				}
			}
			printf("Skipping %d padding bytes\n", i);
			lseek(header.fd, -1, SEEK_CUR);
			continue;
		}
		header.hash = buf[7]<<8|buf[8];

		/* piece name */
		memset(data, '\0', 13);
		if (read(header.fd, data, 12)<12)
			break;

		if (data[0] == 0xff) {
			printf(" [eof]\n");
			break;
		} else printf(" %s\n", data);
		strcpy(header.name, (char *)data);

		if (read(header.fd, buf, 9)<9)
			break;
		memcpy(&header.size, buf,4);
		header.size = ntohl(header.size);
		printf("   offset:  0x%08x\n", (unsigned int)here);
		printf("   size:    %d bytes\n", header.size);
		printf("   hash:    %04x\n", header.hash);
		//printf("BYTE: %02x %02x %02x %02x %02x\n", 
		//	buf[4], buf[5], buf[6], buf[7], buf[8]);
		/* XXX this is not ok */
		//printf("BUF8: %02x\n", buf[8]);
		while (buf[8] && buf[8]>'0' && buf[8] < '9') {
			if (read(header.fd, data, 1)<1)
				break;
			i = data[0];
			if (read(header.fd, data, i)<i)
				break;
			if (data[0]) {
				printf("   version-length: %d\n", i);
				printf("   version: %s\n", data);
				pdata = data;
				while(pdata<data+i) {
					strcat(header.name,pdata==data?"-":",");
					strcat(header.name, (char*)pdata);
					printf("   sub-version: %s\n", pdata);
					pdata=pdata+strlen((char*)pdata)+1;
					for(;*pdata=='\0'&&pdata<data+i;pdata=pdata+1);
				}
			}
			strcpy(header.version, (char *)data);
			if (read(header.fd, buf+8, 1)<1)
				break;
		}
		printf("   name: %s\n", header.name);
		/* callback */
		off = lseek(header.fd, 0, SEEK_CUR);
		printf("   body-at:   0x%08x\n", (unsigned int)off);
		if (fiasco_callback != NULL) {
			fiasco_callback(&header);
			free(header.data);
			continue;
		} else {
		}
		// XXX dup
		lseek(header.fd, off, SEEK_SET);
		lseek(header.fd, header.size, SEEK_CUR);
	}
	return 0;
}

void fiasco_data_read(struct header_t *header)
{
	header->data = (unsigned char *)malloc(header->size);
	if (header->data == NULL) {
		printf("Cannot alloc %d bytes\n", header->size);
		return;
	}
	read(header->fd, header->data, header->size);
}

/* fiasco writer */
int fiasco_new(const char *filename, const char *name)
{
	int fd;
	//int len = htonl(strlen(name)+1+6+14);

	fd = open(filename, O_RDWR|O_CREAT, 0644);
	if (fd == -1)
		return -1;
#if 1
	write(fd, "\xb4\x00\x00\x00\x14\x00\x00\x00\x01\xe8\x0e", 11);
	write(fd, "OSSO UART+USB", 14);
#else
	/* 2nd format doesnt works. atm stay with old one */
	write(fd, "\xb4", 1);
	write(fd, &len, 4);
	/* version header */
	write(fd, "\x00\x00\x00\x02\xe8\x0e", 6);
	/* firmware type */
	write(fd, "OSSO UART+USB", 14);
	/* firmware name */
	write(fd, name, strlen(name));
	write(fd, "", 1);
#endif
	return fd;
}

int fiasco_add_eof(int fd)
{
#if 0
	unsigned char buf[120];
	if (fd == -1)
		return -1;
	memset(buf,'\xff', 120);
	write(fd, buf, 120);
#endif
	return 0;
}


int fiasco_add(int fd, const char *name, const char *file, const char *version)
{
	int gd,ret;
	unsigned int sz;
	unsigned char len;
	unsigned short hash;
	unsigned char *ptr = (unsigned char *)&hash;
	char buf[4096];
	char bname[32];

	if (fd == -1)
		return -1;
	if (file == NULL)
		return -1;
	gd = open(file, O_RDONLY);
	if (gd == -1)
		return -1;

	sz = htonl((unsigned int) lseek(gd, 0, SEEK_END));
	lseek(gd, 0, SEEK_SET);
	// 4 bytes big endian
	//write(fd, "T\x02\x2e\x19\x01\x01\x00", 7); // header?
	write(fd, "T\x01\x2e\x19\x01\x01\x00", 7); // header (old piece format)
	/* checksum */
	hash = do_hash_file(file);
	ptr[0]^=ptr[1]; ptr[1]=ptr[0]^ptr[1]; ptr[0]^=ptr[1];
	write(fd, &hash, 2);
	printf("hash: %04x\n", hash);

	memset(bname, '\0', 13);
	if (name == NULL)
		name = fpid_file(file);
	/* failback */
	if (name == NULL)
		name = file;

	strncpy(bname, name, 12);
	write(fd, bname, 12);

	write(fd, &sz, 4);
	if (version) {
		/* append metadata */
		write(fd, "\x00\x00\x00\x00\x31", 5);
		len = strlen(version);
		write(fd, &len, 1);
		write(fd, version, len);
		write(fd, "\x00\x9b", 2);
	} else {
		write(fd, "\x00\x00\x00\x00\x9b", 5);
	}

	while(1) {
		ret = read(gd, buf, 4096);
		if (ret<1)
			break;
		write(fd, buf, ret);
	}

	return 0;
}

int fiasco_pack(int optind, char *argv[])
{
	const char *file = argv[optind];
	char *type;
	int fd, ret;

	fd = fiasco_new(file, file); // TODO use a format here
	if (fd == -1)
		return 1;

	printf("Package: %s\n", file);
	while((file=argv[++optind])) {
		type = (char *)fpid_file(file);
		printf("Adding %s: %s..\n", type, file);
		ret = fiasco_add(fd, type, file, NULL);
		if (ret<0) {
			printf("Error\n");
			close(fd);
			return 1;
		}
	}
	fiasco_add_eof(fd);
	printf("Done!\n");
	close(fd);
	return 0;
}

/* local code */
#if 0
void my_callback(int fd, struct header_t *header)
{
	fiasco_data_read(header);
	//read(fd, buf, header->size);
	printf("Dumping %s\n", header->name);
	printf("DATA: %02x\n", header->data[0]);
	fiasco_data_free(header);
}

int main(int argc, char **argv)
{
	if (argc!=2) {
		printf("Usage: unfiasco [file]\n");
		return 1;
	}

/*
	fd = fiasco_new("myfiasco", "pancake-edition");
	fiasco_add(fd, "kernel", "zImage", "2.6.22");
	close(fd);
*/

//	fiasco_callback = &my_callback;

	return openfiasco(argv[1]);
}
#endif
