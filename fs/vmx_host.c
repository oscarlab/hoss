#ifdef VMM_GUEST
// Forward FS read/writes to the host instead of the IDE disk.

#include "fs.h"

#include  <inc/vmx.h>
#include <inc/fs.h>
#include <inc/lib.h>

#define HOST_FS_FILE "/vmm/fs.img"

static struct Fd *host_fd;
static union Fsipc host_fsipcbuf __attribute__((aligned(PGSIZE)));

static int
host_fsipc(unsigned type, void *dstva)
{
	ipc_host_send(VMX_HOST_FS_ENV, type, &host_fsipcbuf, PTE_P | PTE_W | PTE_U);
	return ipc_host_recv(dstva);
}


uint64_t
get_host_fd() 
{
	return (uint64_t) host_fd;
}

int
host_read(uint32_t secno, void *dst, size_t nsecs)
{
	int r, read = 0;

	if(host_fd->fd_file.id == 0) {
		host_ipc_init();
	}

	host_fd->fd_offset = secno * SECTSIZE;
	// read from the host, 2 sectors at a time.
	for(; nsecs > 0; nsecs-=2) {

		host_fsipcbuf.read.req_fileid = host_fd->fd_file.id;
		host_fsipcbuf.read.req_n = SECTSIZE * 2;
		if ((r = host_fsipc(FSREQ_READ, NULL)) < 0)
			return r;
		// FIXME: Handle case where r < SECTSIZE * 2;
		memmove(dst+read, &host_fsipcbuf, r);
		read += SECTSIZE * 2;
	}

	return 0;
}

int
host_write(uint32_t secno, const void *src, size_t nsecs)
{
	int r, written = 0;
    
	if(host_fd->fd_file.id == 0) {
		host_ipc_init();
	}

	host_fd->fd_offset = secno * SECTSIZE;
	for(; nsecs > 0; nsecs-=2) {
		host_fsipcbuf.write.req_fileid = host_fd->fd_file.id;
		host_fsipcbuf.write.req_n = SECTSIZE * 2;
		memmove(host_fsipcbuf.write.req_buf, src+written, SECTSIZE * 2);
		if ((r = host_fsipc(FSREQ_WRITE, NULL)) < 0)
			return r;
		written += SECTSIZE * 2;
	}
	return 0;
}

void
host_ipc_init()
{
	int r;
	int vmdisk_number;
	char path_string[50];
	if ((r = fd_alloc(&host_fd)) < 0)
		panic("Couldn't allocate an fd!");
	asm("vmcall":"=a"(vmdisk_number): "0"(VMX_VMCALL_GETDISKIMGNUM));
	snprintf(path_string, 50, "/vmm/fs%d.img", vmdisk_number);
	strcpy(host_fsipcbuf.open.req_path, path_string);
	host_fsipcbuf.open.req_omode = O_RDWR;

	if ((r = host_fsipc(FSREQ_OPEN, host_fd)) < 0) {
		fd_close(host_fd, 0);
		panic("Couldn't open host file!");
	}

}

#endif
