#ifndef W25Q64_H
#define W25Q64_H
#include "stm32f10x.h"

#define W25Q64_TIMEOUT                100
#define W25Q64_GET_HIGH_ADDR(Address) (uint8_t)((Address) >> 16)
#define W25Q64_GET_MID_ADDR(Address)  (uint8_t)((Address) >> 8)
#define W25Q64_GET_LOW_ADDR(Address)  (uint8_t)((Address) >> 0)
#define W25Q64_BLOCK_NUM              (uint32_t)128
#define W25Q64_BLOCK_SIZE             (uint32_t)(64 * 1024)
#define W25Q64_GET_BLOCK_ADDR(block)  (uint32_t)(block) * W25Q64_BLOCK_SIZE
#define W25Q64_SECTOR_NUM             (uint32_t)16
#define W25Q64_PAGE_NUM               (uint32_t)(W25Q64_BLOCK_NUM * W25Q64_SECTOR_NUM * 16)
#define W25Q64_PAGE_NUM_PRE_BLOCK     (uint32_t)256
#define W25Q64_PAGE_SIZE              (uint32_t)256
#define W25Q64_GET_PAGE_ADDR(block, page)                                                          \
    (uint32_t)(W25Q64_GET_BLOCK_ADDR((block)) + (page) * W25Q64_PAGE_SIZE)

void W25Q64_Init(void);
void W25Q64_ReadID(uint8_t *MID, uint16_t *DID);
void W25Q64_SectorErase(uint32_t Address);
void W25Q64_BlockErase_64K(uint32_t Address);
void W25Q64_WritePage(uint32_t Address, uint8_t *DataBuf, uint32_t DataBufLen);
void W25Q64_ReadData(uint32_t Address, uint8_t *RecvBuf, uint32_t RecvBufLen, uint32_t DataLen);
void W25Q64_ReadPages(uint32_t startPageAddress,
                      uint8_t *RecvBuf,
                      uint32_t RecvBufLen,
                      uint16_t pageNum);
void ExFlash_DownloadFromUSART(void);
void ExFlash_MoveInAPP(void);
void ExFlash_DeleteProg(void);
#endif
