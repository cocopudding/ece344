#include "testfs.h"
#include "list.h"
#include "super.h"
#include "block.h"
#include "inode.h"

/* given logical block number, read the corresponding physical block into block.
 * return physical block number.
 * returns 0 if physical block does not exist.
 * returns negative value on other errors. */
#define MAX_SIZE  34376597504
static int
testfs_read_block(struct inode *in, int log_block_nr, char *block)
{
	int phy_block_nr = 0;

	assert(log_block_nr >= 0);
	if (log_block_nr < NR_DIRECT_BLOCKS) {
		phy_block_nr = (int)in->in.i_block_nr[log_block_nr];
	} 
        else {
		log_block_nr -= NR_DIRECT_BLOCKS;

		if (log_block_nr >= NR_INDIRECT_BLOCKS)
                {
			//TBD();
                    log_block_nr -= NR_INDIRECT_BLOCKS;
                    if(log_block_nr>= NR_INDIRECT_BLOCKS*NR_INDIRECT_BLOCKS)
                        return -EFBIG;
                    //need to look at doubly indirect blocks
                    if(in->in.i_dindirect >0)
                    {
                        read_blocks(in->sb,block,in->in.i_dindirect,1);
                        //get the indirect blocks now
                        if(((int*)block)[log_block_nr/NR_INDIRECT_BLOCKS]>0)
                        {
                            read_blocks(in->sb,block,((int*)block)[log_block_nr/NR_INDIRECT_BLOCKS],1);
                            phy_block_nr = ((int *)block)[log_block_nr%NR_INDIRECT_BLOCKS];
                            
                        }
                        else
                            return ((int*)block)[log_block_nr/NR_INDIRECT_BLOCKS];
                    }
                    
                }
		else if (in->in.i_indirect > 0) {
			read_blocks(in->sb, block, in->in.i_indirect, 1);
			phy_block_nr = ((int *)block)[log_block_nr];
		}
	}
	if (phy_block_nr > 0) {
		read_blocks(in->sb, block, phy_block_nr, 1);
	} else {
		/* we support sparse files by zeroing out a block that is not
		 * allocated on disk. */
		bzero(block, BLOCK_SIZE);
	}
	return phy_block_nr;
}

int
testfs_read_data(struct inode *in, char *buf, off_t start, size_t size)
{
	char block[BLOCK_SIZE];
	long block_nr = start / BLOCK_SIZE;
	long block_ix = start % BLOCK_SIZE;
	int ret;

	assert(buf);
	if (start + (off_t) size > in->in.i_size) {
		size = in->in.i_size - start;
	}
	if (block_ix + size > BLOCK_SIZE) {
            if(size>MAX_SIZE)
                return -EFBIG;
            // the case when the size need to be read exceeds the current block
            long read_block_nr = DIVROUNDUP(size+block_ix, BLOCK_SIZE);
            int i = 0;
            long size_read = 0;
            while(i<read_block_nr)
            {
                //if it is reading the first block
                if(i==0)
                {
                    if ((ret = testfs_read_block(in, block_nr, block)) < 0)
                        return ret;
                    memcpy(buf, block + block_ix, BLOCK_SIZE - block_ix);
                    size_read = size_read + BLOCK_SIZE - block_ix;
                }
                //if it is reading the last block
                else if(i == read_block_nr-1)
                {
                    if ((ret = testfs_read_block(in, block_nr+i, block)) < 0)
                        return ret;
                    memcpy(buf+size_read, block,size-size_read);  
                    return size;
                }
                //else we're reading the middle blocks, meaning the entire block
                else
                {
                    if ((ret = testfs_read_block(in, block_nr+i, block)) < 0)
                        return ret;
                    memcpy(buf+size_read, block,BLOCK_SIZE);  
                    size_read = size_read + BLOCK_SIZE;
                }
                i++;
            }
	}
	if ((ret = testfs_read_block(in, block_nr, block)) < 0)
		return ret;
	memcpy(buf, block + block_ix, size);
	/* return the number of bytes read or any error */
	return size;
}

/* given logical block number, allocate a new physical block, if it does not
 * exist already, and return the physical block number that is allocated.
 * returns negative value on error. */
static int
testfs_allocate_block(struct inode *in, int log_block_nr, char *block)
{
	int phy_block_nr;
	char indirect[BLOCK_SIZE];
        char dindirect[BLOCK_SIZE];
	int indirect_allocated = 0;
        int dindirect_allocated = 0;

	assert(log_block_nr >= 0);
        if(log_block_nr>(MAX_SIZE/BLOCK_SIZE))
            return -EFBIG;
	phy_block_nr = testfs_read_block(in, log_block_nr, block);

	/* phy_block_nr > 0: block exists, so we don't need to allocate it, 
	   phy_block_nr < 0: some error */
	if (phy_block_nr != 0)
		return phy_block_nr;

	/* allocate a direct block */
	if (log_block_nr < NR_DIRECT_BLOCKS) {
		assert(in->in.i_block_nr[log_block_nr] == 0);
		phy_block_nr = testfs_alloc_block_for_inode(in);
		if (phy_block_nr >= 0) {
			in->in.i_block_nr[log_block_nr] = phy_block_nr;
		}
		return phy_block_nr;
	}

	log_block_nr -= NR_DIRECT_BLOCKS;
        //when we need to allocate an doubly indirect block
	if (log_block_nr >= NR_INDIRECT_BLOCKS)
		//TBD();
        {
            log_block_nr -= NR_INDIRECT_BLOCKS;
            //allocate a doubly indirect block if doesnt exist
            if (in->in.i_dindirect == 0)
            {
                bzero(dindirect,BLOCK_SIZE);
                phy_block_nr = testfs_alloc_block_for_inode(in);
                if (phy_block_nr < 0)
                    return phy_block_nr;
                dindirect_allocated = 1;
                in->in.i_dindirect = phy_block_nr;              
            }
            //otherwise read the dindirect block
            else
            {
                read_blocks(in->sb,dindirect,in->in.i_dindirect,1);
            }
            
            //allocate an indirect block if it doesnt exist
            if(((int*)dindirect)[log_block_nr/NR_INDIRECT_BLOCKS] == 0)
            {
                bzero(indirect,BLOCK_SIZE);
                phy_block_nr = testfs_alloc_block_for_inode(in);
                if (phy_block_nr >= 0)
                {
                    ((int *)dindirect)[log_block_nr/NR_INDIRECT_BLOCKS] = phy_block_nr; 
                    indirect_allocated = 1;
                }
                else if (dindirect_allocated)
                {
                    testfs_free_block_from_inode(in,in->in.i_dindirect);
                    return phy_block_nr;
                }   
            }
            //otherwise the indirect block exists already
            else
            {
                read_blocks(in->sb,indirect,((int *)dindirect)[log_block_nr/NR_INDIRECT_BLOCKS],1);
            }
            //now we have the indirect block,we need to allocate a direct block
            assert(((int *)indirect)[log_block_nr%NR_INDIRECT_BLOCKS]==0);
            phy_block_nr = testfs_alloc_block_for_inode(in);
            if(phy_block_nr >= 0)
            {
                ((int *)indirect)[log_block_nr%NR_INDIRECT_BLOCKS] = phy_block_nr;
                write_blocks(in->sb,indirect, ((int *)dindirect)[log_block_nr/NR_INDIRECT_BLOCKS],1);
                write_blocks(in->sb, dindirect, in->in.i_dindirect, 1); 
            }
            else
            {
                if(indirect_allocated)
                {
                    testfs_free_block_from_inode(in,((int *)dindirect)[log_block_nr/NR_INDIRECT_BLOCKS]);
                }
                if(dindirect_allocated)
                {
                    testfs_free_block_from_inode(in,in->in.i_dindirect);
                }
            }
            
            	return phy_block_nr;
        }

        else{
            if (in->in.i_indirect == 0) {	/* allocate an indirect block */
                    bzero(indirect, BLOCK_SIZE);
                    phy_block_nr = testfs_alloc_block_for_inode(in);
                    if (phy_block_nr < 0)
                            return phy_block_nr;
                    indirect_allocated = 1;
                    in->in.i_indirect = phy_block_nr;
            } else {	/* read indirect block */
                    read_blocks(in->sb, indirect, in->in.i_indirect, 1);
            }

            /* allocate direct block */
            assert(((int *)indirect)[log_block_nr] == 0);	
            phy_block_nr = testfs_alloc_block_for_inode(in);

            if (phy_block_nr >= 0) {
                    /* update indirect block */
                    ((int *)indirect)[log_block_nr] = phy_block_nr;
                    write_blocks(in->sb, indirect, in->in.i_indirect, 1);
            } else if (indirect_allocated) {
                    /* free the indirect block that was allocated */
                    testfs_free_block_from_inode(in, in->in.i_indirect);
            }
            return phy_block_nr;
        }
}

int
testfs_write_data(struct inode *in, const char *buf, off_t start, size_t size)
{
	char block[BLOCK_SIZE];
	long block_nr = start / BLOCK_SIZE;
	long block_ix = start % BLOCK_SIZE;
	int ret;

	if (block_ix + size > BLOCK_SIZE) {
	  // when we need to read multiple blocks
            long write_block_nr = DIVROUNDUP(size+block_ix, BLOCK_SIZE);
            int i = 0;
            // long original_block_ix = block_ix;
            long size_read = 0;
            while(i<write_block_nr)
            {
                // when writing the first block
                if(i ==0)
                {
                    ret = testfs_allocate_block(in, block_nr, block);
	            if (ret < 0)
                        return ret;
                    memcpy(block + block_ix, buf, BLOCK_SIZE - block_ix);
                    write_blocks(in->sb, block, ret, 1);
                    size_read = size_read + (BLOCK_SIZE - block_ix);
                }
                //when writing the last block
                else if(i == write_block_nr-1)
                {
                    ret = testfs_allocate_block(in, block_nr+i, block);
                    if(ret < 0)
                    {
                        if (ret == -EFBIG)
                        {
                        in->in.i_size = MAX_SIZE;
                        in->i_flags |= I_FLAGS_DIRTY;                       
                        }
                        return ret;
                    }
                    memcpy(block, buf+size_read, size-size_read);    
                    write_blocks(in->sb, block, ret, 1);
                    if (size > 0)
                        in->in.i_size = MAX(in->in.i_size, start + (off_t) size);
                    in->i_flags |= I_FLAGS_DIRTY;
                    return size;
                }
                //when writing to middle blocks
                else
                {
                    ret = testfs_allocate_block(in, block_nr+i, block);
	            if (ret < 0)
                        return ret;
                    memcpy(block, buf+size_read,BLOCK_SIZE);  
                    write_blocks(in->sb, block, ret, 1); 
                    size_read = size_read + BLOCK_SIZE;
                }
                i++;                                   
            }
	}
	/* ret is the newly allocated physical block number */
	ret = testfs_allocate_block(in, block_nr, block);
	if (ret < 0)
		return ret;
	memcpy(block + block_ix, buf, size);
	write_blocks(in->sb, block, ret, 1);
	/* increment i_size by the number of bytes written. */
	if (size > 0)
		in->in.i_size = MAX(in->in.i_size, start + (off_t) size);
	in->i_flags |= I_FLAGS_DIRTY;
	/* return the number of bytes written or any error */
	return size;
}

int
testfs_free_blocks(struct inode *in)
{
	int i;
	int e_block_nr;

	/* last block number */
	e_block_nr = DIVROUNDUP(in->in.i_size, BLOCK_SIZE);

	/* remove direct blocks */
	for (i = 0; i < e_block_nr && i < NR_DIRECT_BLOCKS; i++) {
		if (in->in.i_block_nr[i] == 0)
			continue;
		testfs_free_block_from_inode(in, in->in.i_block_nr[i]);
		in->in.i_block_nr[i] = 0;
	}
	e_block_nr -= NR_DIRECT_BLOCKS;

	/* remove indirect blocks */
	if (in->in.i_indirect > 0) {
		char block[BLOCK_SIZE];
		read_blocks(in->sb, block, in->in.i_indirect, 1);
		for (i = 0; i < e_block_nr && i < NR_INDIRECT_BLOCKS; i++) 
                {
			testfs_free_block_from_inode(in, ((int *)block)[i]);
			((int *)block)[i] = 0;
		}
		testfs_free_block_from_inode(in, in->in.i_indirect);
		in->in.i_indirect = 0;
	}

	e_block_nr -= NR_INDIRECT_BLOCKS;
	if (e_block_nr >= 0 && in->in.i_dindirect>0) {
		//TBD();
            char dindirect_block[BLOCK_SIZE];
            read_blocks(in->sb,dindirect_block,in->in.i_dindirect,1);
            for(i = 0; e_block_nr>0 && i<NR_INDIRECT_BLOCKS;i++)
            {
                if(((int*)dindirect_block)[i]>0)
                {
                    char indirect_block[BLOCK_SIZE];
                    read_blocks(in->sb, indirect_block, ((int *)dindirect_block)[i],1);
                    int j;
                    for (j=0; j<e_block_nr && j<NR_INDIRECT_BLOCKS; j++)
                    {
                        testfs_free_block_from_inode(in,((int*)indirect_block)[j]);
                        ((int*)indirect_block)[j] = 0;
                    }
                    e_block_nr = e_block_nr - j;
                    testfs_free_block_from_inode(in, ((int*)dindirect_block)[i]);
                    ((int*)dindirect_block)[i] = 0;        
                }
                else
                    e_block_nr -= NR_INDIRECT_BLOCKS;
            }
            testfs_free_block_from_inode(in,in->in.i_dindirect);
            in->in.i_dindirect = 0;
	}

	in->in.i_size = 0;
	in->i_flags |= I_FLAGS_DIRTY;
	return 0;
}