#include "aes.h"
#include <stdio.h>
#include <string.h>
extern void * aes_decrypt_init(const u8 *, size_t);
extern void * aes_decrypt(void *, const u8 *, u8 *);
extern void * aes_decrypt_deinit(void *);

int aes_128_cbc_decrypt(const u8 *key, const u8 *iv, u8 *data, size_t data_len)
{
	void *ctx;
	u8 cbc[AES_BLOCK_SIZE], tmp[AES_BLOCK_SIZE];
	u8 *pos = data;
	int i, j, blocks;

	ctx = aes_decrypt_init(key, 16);
	if (ctx == NULL)
		return -1;
	memcpy(cbc, iv, AES_BLOCK_SIZE);

	blocks = data_len / AES_BLOCK_SIZE;
	for (i = 0; i < blocks; i++) {
		memcpy(tmp, pos, AES_BLOCK_SIZE);
		aes_decrypt(ctx, pos, pos);
		for (j = 0; j < AES_BLOCK_SIZE; j++)
			pos[j] ^= cbc[j];
		memcpy(cbc, tmp, AES_BLOCK_SIZE);
		pos += AES_BLOCK_SIZE;
	}
	aes_decrypt_deinit(ctx);
	return 0;
}
