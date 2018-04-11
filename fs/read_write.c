#include "testfs.h"
#include "list.h"
#include "super.h"
#include "block.h"
#include "inode.h"


/* given logical block number, read the corresponding physical block into block.
 * return physical block number.
 * returns 0 if physical block does not exist.
 * returns negative value on other errors. */
static int
testfs_read_block(struct inode *in, int log_block_nr, char *block)
{
	int phy_block_nr = 0;

	assert(log_block_nr >= 0);
	if (log_block_nr < NR_DIRECT_BLOCKS) {
		phy_block_nr = (int)in->in.i_block_nr[log_block_nr];
	} else {
		log_block_nr -= NR_DIRECT_BLOCKS;

		if (log_block_nr >= NR_INDIRECT_BLOCKS&&in->in.i_dindirect > 0){
                    char block2[BLOCK_SIZE];
                   read_blocks(in->sb, block2, in->in.i_dindirect, 1);
                    int dindirectindex=(int)((log_block_nr-NR_INDIRECT_BLOCKS)/NR_INDIRECT_BLOCKS);
                    int directindex=((int)((log_block_nr-NR_INDIRECT_BLOCKS)%NR_INDIRECT_BLOCKS));
                    
                    phy_block_nr = ((int *)block2)[dindirectindex];
                    if (phy_block_nr > 0) {
                        read_blocks(in->sb, block, phy_block_nr, 1);
                        phy_block_nr = ((int *)block)[directindex];
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
        
        
        if(size>(size_t)(34376597504-start)){
            //size = (34376597504-start);
            return -EFBIG;
        }

	assert(buf);
	if (start + (off_t) size > in->in.i_size) {
		size = in->in.i_size - start;
	}
	if (block_ix + size > BLOCK_SIZE) {
            if(size>34376597504)
                return -EFBIG;
		//TBD();
            //read first section
            if ((ret = testfs_read_block(in, block_nr, block)) < 0)
		return ret;
            memcpy(buf, block + block_ix, BLOCK_SIZE-block_ix);
            buf+=BLOCK_SIZE-block_ix;
            
            //check if there is full section in the middle 
            long tempSize=size-(BLOCK_SIZE-block_ix);
            int i=0;
            if(size-(BLOCK_SIZE-block_ix)>BLOCK_SIZE){
               // long location=block+BLOCK_SIZE;
                while(tempSize>BLOCK_SIZE){
                    i++;
                    if ((ret = testfs_read_block(in, block_nr+i, block)) < 0)
                        return ret;
                    memcpy(buf, block,BLOCK_SIZE);
                    buf+=BLOCK_SIZE;
                    tempSize-=BLOCK_SIZE;
                }
                
            }
            
            if(i==0){
                //no full section in the middle, just read left over
                if ((ret = testfs_read_block(in, block_nr+1, block)) < 0)
                   return ret;
               memcpy(buf, block, size-(BLOCK_SIZE-block_ix));
               buf+=size-(BLOCK_SIZE-block_ix);
            }
            else{
                 if ((ret = testfs_read_block(in, block_nr+i+1, block)) < 0)
                    return ret;
                memcpy(buf, block, size-(BLOCK_SIZE-block_ix)-i*BLOCK_SIZE);
                buf+=size-(BLOCK_SIZE-block_ix)-i*BLOCK_SIZE;
            }

            return size;
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
        int dindirect_allocated=0;

	assert(log_block_nr >= 0);
        if (log_block_nr >= 4196362)
            return -EFBIG; 
	phy_block_nr = testfs_read_block(in, log_block_nr, block);

	/* phy_block_nr > 0: block exists, so we don't need to allocate it, 
	   phy_block_nr < 0: some error */
	if (phy_block_nr != 0){
            return phy_block_nr;
        } 

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
	if (log_block_nr >= NR_INDIRECT_BLOCKS){
            
        //for doubly indirect block TBD
            if(in->in.i_dindirect==0){/* allocate a double indirect block */
                bzero(dindirect, BLOCK_SIZE);
                phy_block_nr = testfs_alloc_block_for_inode(in);
                if (phy_block_nr < 0)
			return phy_block_nr;
                dindirect_allocated = 1;
		in->in.i_dindirect = phy_block_nr;
            }
            else{/* read double indirect block */
                read_blocks(in->sb, dindirect, in->in.i_dindirect, 1);
            }
         
            /* allocate indirect block if needed */
            int dindirectindex=((int)(log_block_nr-NR_INDIRECT_BLOCKS))/NR_INDIRECT_BLOCKS;
            int directindex=((int)(log_block_nr-NR_INDIRECT_BLOCKS))%NR_INDIRECT_BLOCKS;
          //  assert(((int *)dindirect)[dindirectindex] == 0);
           
            
            if (((int *)dindirect)[dindirectindex]==0) {
                bzero(indirect, BLOCK_SIZE);
                 phy_block_nr = testfs_alloc_block_for_inode(in);
                 if(phy_block_nr>=0){
                    /* update double indirect block */
                   ((int *)dindirect)[dindirectindex] = phy_block_nr;
                   indirect_allocated=1;
                   write_blocks(in->sb, indirect, ((int *)dindirect)[dindirectindex], 1);
                   write_blocks(in->sb, dindirect, in->in.i_dindirect, 1);
                   
                 }
                 else{
                     if(dindirect_allocated==1){
                      testfs_free_block_from_inode(in, in->in.i_dindirect);
                      in->in.i_dindirect = 0; 
                     }
                     return phy_block_nr;
                 } 
            } 
            else{
                //((int *)indirect)[0]=((int *)dindirect)[dindirectindex];
               read_blocks(in->sb, indirect, ((int *)dindirect)[dindirectindex], 1);
           }
        
            /* allocate direct block */
	//assert(((int *)indirect)[directindex] == 0);	
	phy_block_nr = testfs_alloc_block_for_inode(in);

	if (phy_block_nr >= 0) {
		/* update indirect block */
		((int *)indirect)[directindex] = phy_block_nr;
                write_blocks(in->sb, indirect, ((int *)dindirect)[dindirectindex], 1);
		write_blocks(in->sb, dindirect, in->in.i_dindirect, 1);
	} else{
            if(indirect_allocated==1 && !dindirect_allocated){
                 testfs_free_block_from_inode(in, ((int*) dindirect)[dindirectindex]);
            }else if (dindirect_allocated){
                testfs_free_block_from_inode(in, ((int*) dindirect)[dindirectindex]);
                testfs_free_block_from_inode(in, in->in.i_dindirect);
                in->in.i_dindirect = 0; 
            }
        }
	return phy_block_nr;
            
        }

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
                in->in.i_indirect = 0; 
	}
	return phy_block_nr;
}

int
testfs_write_data(struct inode *in, const char *buf, off_t start, size_t size)
{
	char block[BLOCK_SIZE];
	long block_nr = start / BLOCK_SIZE;
	long block_ix = start % BLOCK_SIZE;
	int ret;
        int check=0;
        
       /* if(size>(size_t)(34376597504-start)){
            size=(size_t)(34376597504-start);
            check=1;
        }*/
	if (block_ix + size > BLOCK_SIZE) {
            ///////////////////////////////
            if(size>(size_t)(34376597504-start)){
            size=(size_t)(34376597504-start);
            check=1;
        }
            ret = testfs_allocate_block(in, block_nr, block);
            if (ret < 0){
		return ret;
            } 
            memcpy(block + block_ix, buf, BLOCK_SIZE-block_ix);
            buf+=BLOCK_SIZE-block_ix;
            write_blocks(in->sb, block, ret, 1);

            
            //check if there is full section in the middle 
            long tempSize=size-(BLOCK_SIZE-block_ix);
            int i=0;
            if(size-(BLOCK_SIZE-block_ix)>BLOCK_SIZE){
                while(tempSize>BLOCK_SIZE){
                      i++;
                    ret = testfs_allocate_block(in, block_nr+i, block);
                    if (ret < 0){
                        // assert(ret==-ENOSPC); 
                        // fprintf(stderr,"Checkpoint 2 \n"); 
                        return ret;
                    } 
                    memcpy(block, buf, BLOCK_SIZE);
                    buf+=BLOCK_SIZE;
                    write_blocks(in->sb, block, ret, 1);
                    tempSize-=BLOCK_SIZE;
                }
                
            }
            
            if(i==0){
                //no full section in the middle, just read left over
                ret = testfs_allocate_block(in, block_nr+1, block);
                if (ret < 0){
                    // assert(ret==-ENOSPC); 
                    // fprintf(stderr,"Checkpoint 3 \n"); 
                    // return ret;
                    goto out; 
                } 
                memcpy(block, buf, size-(BLOCK_SIZE-block_ix));
                buf+=size-(BLOCK_SIZE-block_ix);
                write_blocks(in->sb, block, ret, 1);
            }else{
                 ret = testfs_allocate_block(in, block_nr+i+1, block);
                if (ret < 0){
                    // assert(ret==-ENOSPC); 
                    return ret;          
                }
                    
                memcpy(block, buf, size-(BLOCK_SIZE-block_ix)-i*BLOCK_SIZE);
                write_blocks(in->sb, block, ret, 1);
                buf+=(size-(BLOCK_SIZE-block_ix)-i*BLOCK_SIZE);
            }
            
            goto out;
            //////////////////
	}
	/* ret is the newly allocated physical block number */
	ret = testfs_allocate_block(in, block_nr, block);
	if (ret < 0){
            // assert(ret==-ENOSPC); 
            // fprintf(stderr,"Checkppoint 4 \n"); 
            return ret;
        } 
	memcpy(block + block_ix, buf, size);
	write_blocks(in->sb, block, ret, 1);
	/* increment i_size by the number of bytes written. */
out:
        
        if (size > 0)
		in->in.i_size = MAX(in->in.i_size, start + (off_t) size);
	in->i_flags |= I_FLAGS_DIRTY;
        
        if(check==1){
          size= -EFBIG;
        }
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
                    for (i = 0; i < e_block_nr && i < NR_INDIRECT_BLOCKS; i++) {
                            testfs_free_block_from_inode(in, ((int *)block)[i]);
                            ((int *)block)[i] = 0;
                    }
                    testfs_free_block_from_inode(in, in->in.i_indirect);
                    in->in.i_indirect = 0;
            }

            e_block_nr -= NR_INDIRECT_BLOCKS;
            
            // double indirect 
            if (e_block_nr > 0&&in->in.i_dindirect > 0) {
                char dblock[BLOCK_SIZE];
                char block[BLOCK_SIZE];
                read_blocks(in->sb, dblock, in->in.i_dindirect, 1);
                int j=0;

                  for (i = 0; i < NR_INDIRECT_BLOCKS&&j<e_block_nr; i++) {
                      if(((int *)dblock)[i]>0){
                        read_blocks(in->sb, block, ((int *)dblock)[i], 1);
                        int k=0;
                        while(k<NR_INDIRECT_BLOCKS){
                            if(((int *)block)[k]>0){
                                   testfs_free_block_from_inode(in, ((int *)block)[k]);
                                  ((int *)block)[k] = 0;
                            }
                          k++;
                         j++;
                        }
                        testfs_free_block_from_inode(in, ((int *)dblock)[i]);
                        ((int *)dblock)[i] = 0;
                      }//else
                        //  e_block_nr -= NR_INDIRECT_BLOCKS;
                }

                    testfs_free_block_from_inode(in, in->in.i_dindirect);
                    in->in.i_dindirect = 0;

            }

            in->in.i_size = 0;
            in->i_flags |= I_FLAGS_DIRTY;
            return 0;
    }
