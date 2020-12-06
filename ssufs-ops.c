#include "ssufs-ops.h"

extern struct filehandle_t file_handle_array[MAX_OPEN_FILES];

int ssufs_allocFileHandle()
{
	for (int i = 0; i < MAX_OPEN_FILES; i++)
	{
		if (file_handle_array[i].inode_number == -1)
		{
			return i;
		}
	}
	return -1;
}

int ssufs_create(char *filename)
{
	struct inode_t tmp;
	int inodenum;
	int datablock;

	if (open_namei(filename) != -1)
		return -1;
	if ((inodenum = ssufs_allocInode()) == -1)
		return -1;

	ssufs_readInode(inodenum, &tmp);
	strcpy(tmp.name, filename);
	tmp.status = INODE_IN_USE;
	ssufs_writeInode(inodenum, &tmp);
	return inodenum;
}

void ssufs_delete(char *filename)
{
	int inodenum, i;
	if ((inodenum = open_namei(filename)) == -1)
		return;

	for (i = 0; i < MAX_OPEN_FILES; ++i)
		if (file_handle_array[i].inode_number == inodenum)
		{
			file_handle_array[i].inode_number = -1;
			file_handle_array[i].offset = 0;
			break;
		}

	ssufs_freeInode(inodenum);
}

int ssufs_open(char *filename)
{
	int inodenum, filehandle;
	if ((inodenum = open_namei(filename)) == -1)
		return -1;

	if ((filehandle = ssufs_allocFileHandle()) == -1)
		return -1;

	file_handle_array[filehandle].inode_number = inodenum;

	return filehandle;
}

void ssufs_close(int file_handle)
{
	file_handle_array[file_handle].inode_number = -1;
	file_handle_array[file_handle].offset = 0;
}

int ssufs_read(int file_handle, char *buf, int nbytes)
{
	struct inode_t tmp;
	struct filehandle_t *file;
	char block[BLOCKSIZE];
	int cur = 0, cur_block = 0;

	if (file_handle < 0 && file_handle >= MAX_OPEN_FILES)
		return -1;

	file = &file_handle_array[file_handle];
	ssufs_readInode(file->inode_number, &tmp);
	if (tmp.file_size < nbytes + file->offset)
		return -1;

	while (cur < nbytes)
	{
		int blocknum;
		int block_offset = (cur + file->offset) % BLOCKSIZE;
		int n = 0;
		cur_block = (cur + file->offset) / BLOCKSIZE;
		memset(block, 0, BLOCKSIZE);
		if (tmp.direct_blocks[cur_block] == -1)
			return -1;
		else
		{
			ssufs_readDataBlock(tmp.direct_blocks[cur_block], block);
			blocknum = tmp.direct_blocks[cur_block];
		}
		n = BLOCKSIZE - block_offset;
		if (cur + n > nbytes)
			n -= cur + n - nbytes;
		memcpy(buf + cur, block + block_offset, n);
		cur += n;
	}
	file->offset += nbytes;
	ssufs_writeInode(file->inode_number, &tmp);
	return 0;
}

int ssufs_write(int file_handle, char *buf, int nbytes)
{
	struct inode_t tmp;
	struct filehandle_t *file;
	char block[BLOCKSIZE];
	int cur = 0, cur_block = 0;
	int less_size = 0;
	int last_block = 0;

	if (file_handle < 0 && file_handle >= MAX_OPEN_FILES)
		return -1;

	file = &file_handle_array[file_handle];
	ssufs_readInode(file->inode_number, &tmp);

	for (int i = 0; i < MAX_FILE_SIZE; ++i)
		if (tmp.direct_blocks[i] == -1)
		{
			last_block = i - 1;
			break;
		}
	less_size = (MAX_FILE_SIZE * BLOCKSIZE) - tmp.file_size + (tmp.file_size - file->offset);
	if (less_size < nbytes)
		return -1;

	while (cur < nbytes)
	{
		int blocknum;
		int block_offset = (cur + file->offset) % BLOCKSIZE;
		int n = 0;
		cur_block = (cur + file->offset) / BLOCKSIZE;
		memset(block, 0, BLOCKSIZE);
		if (tmp.direct_blocks[cur_block] == -1)
		{
			blocknum = ssufs_allocDataBlock();
			if (blocknum == -1)
			{
				for (int i = last_block + 1; i < cur_block; ++i)
					ssufs_freeDataBlock(tmp.direct_blocks[i]);
				return -1;
			}
		}
		else
		{
			ssufs_readDataBlock(tmp.direct_blocks[cur_block], block);
			blocknum = tmp.direct_blocks[cur_block];
		}
		n = BLOCKSIZE - block_offset;
		if (cur + n > nbytes)
			n -= cur + n - nbytes;
		memcpy(block + block_offset, buf + cur, n);
		ssufs_writeDataBlock(blocknum, block);
		cur += n;
		tmp.file_size += n;
		tmp.direct_blocks[cur_block] = blocknum;
	}
	file->offset += nbytes;
	ssufs_writeInode(file->inode_number, &tmp);
	return 0;
}

int ssufs_lseek(int file_handle, int nseek)
{
	int offset = file_handle_array[file_handle].offset;

	struct inode_t *tmp = (struct inode_t *)malloc(sizeof(struct inode_t));
	ssufs_readInode(file_handle_array[file_handle].inode_number, tmp);

	int fsize = tmp->file_size;

	offset += nseek;

	if ((fsize == -1) || (offset < 0) || (offset > fsize))
	{
		free(tmp);
		return -1;
	}

	file_handle_array[file_handle].offset = offset;
	free(tmp);

	return 0;
}
