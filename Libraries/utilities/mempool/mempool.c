/**
  ******************************************************************************
  * @file    mempool.c
  * @author  YANDLD
  * �ο�RTT mempoolʵ��
  * @version V2.5
  * @date    2013.12.25
  * @brief   www.beyondcore.net   http://upcmcu.taobao.com 
  ******************************************************************************
  */
  
#include "mempool.h"
#include <stdint.h>
#include <stdio.h>

int mp_init(struct mempool *mp, const char *name, void *start, size_t size, size_t block_size)
{
    uint8_t *block_ptr;
    uint32_t offset;
    
    /* initialize memory pool */
    mp->start_address = start;
    mp->size = RT_ALIGN_DOWN(size, 4); 
    
    /* align the block size */
    block_size = RT_ALIGN(block_size, 4);
    mp->block_size = block_size;
    
    /* align to align size byte */
    mp->block_total_count = mp->size / (mp->block_size + sizeof(uint8_t *));
    mp->block_free_count  = mp->block_total_count;

    /* initialize free block list */
    // ����һ����ʽ��
    block_ptr = (uint8_t *)mp->start_address;
    for (offset = 0; offset < mp->block_total_count; offset ++)
    {
        *(uint8_t **)(block_ptr + offset * (block_size + sizeof(uint8_t *))) =
        (uint8_t *)(block_ptr + (offset + 1) * (block_size + sizeof(uint8_t *)));
    }
    //�����һ����ʽ����ֹ
    *(uint8_t **)(block_ptr + (offset - 1) * (block_size + sizeof(uint8_t *))) = NULL;
    //ָ���һ��block
    mp->block_list = block_ptr;

    return 0;
}
                    
void *mp_alloc(mp_t mp)
{
    uint8_t *block_ptr;
    
    if (mp->block_free_count)
    {
        /* memory block is available. decrease the free block counter */
        mp->block_free_count --;
        
        /* get block from block list */
        block_ptr      = mp->block_list;
        mp->block_list = *(uint8_t **)block_ptr;

        /* point to memory pool */
        //ÿ�����blockûmalloc��֮�� �����ǰ��ָ�����ʱָ��mp�ṹ��
        *(uint8_t **)block_ptr = (uint8_t *)mp;
        return (uint8_t *)(block_ptr + sizeof(uint8_t *));
    }
    else
    {
        return NULL;
    }
}


void mp_free(void *block)
{
    uint8_t **block_ptr;
    struct mempool *mp;
    
    /* get the control block of pool which the block belongs to */
    // ��malloc�����ߵ�block ǰ��ͷָ��һ������ָ��mempool�ṹ��
    block_ptr = (uint8_t **)((uint8_t *)block - sizeof(uint8_t *));
    mp        = (struct mempool *)*block_ptr;

    /* increase the free block count */
    mp->block_free_count ++;

    /* link the block into the block list */
    //�Ѹո��ͷŵ������ͷָ��ָ����һ�����block
    *block_ptr = mp->block_list;
    //��һ��malloc ��ȡ�����block(������block����)
    mp->block_list = (uint8_t *)block_ptr;
}

