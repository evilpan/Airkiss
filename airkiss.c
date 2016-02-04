#include "airkiss.h"
#include <stdio.h>
#include <string.h>

#define PASSWORD_MAX_LEN	32
#define ESSID_MAX_LEN		32

//Only save password and random
#define USR_DATA_BUFF_MAX_SIZE	(PASSWORD_MAX_LEN + 1)
typedef enum
{
	AIRKISS_STATE_STOPED = 0,
	AIRKISS_STATE_IDLE,
	AIRKISS_STATE_SRC_LOCKED,
	AIRKISS_STATE_MAGIC_CODE_COMPLETE,
	AIRKISS_STATE_PREFIX_CODE_COMPLETE,
	AIRKISS_STATE_COMPLETE
} AIR_KISS_STATE;

#define MAX_GUIDE_RECORD	4
typedef struct
{
	unsigned short  length_record[MAX_GUIDE_RECORD + 1];
}guide_code_record;

#define MAX_MAGIC_CODE_RECORD	4
typedef struct
{
	unsigned short record[MAX_MAGIC_CODE_RECORD + 1];
}magic_code_record;

#define MAX_PREFIX_CODE_RECORD	4
typedef struct
{
	unsigned short record[MAX_PREFIX_CODE_RECORD + 1];
}prfix_code_record;

#define MAX_SEQ_CODE_RECORD	6
typedef struct
{
	unsigned short record[MAX_SEQ_CODE_RECORD + 1];
}seq_code_record;

union airkiss_data{
		guide_code_record guide_code;
		magic_code_record magic_code;
		prfix_code_record prefix_code;
		seq_code_record  seq_code;
};

typedef struct
{
	char* pwd;						
	char* ssid;
	unsigned char pswd_len;
	unsigned char ssid_len;
	unsigned char random_num;
	unsigned char ssid_crc;
	unsigned char usr_data[USR_DATA_BUFF_MAX_SIZE];
	AIR_KISS_STATE airkiss_state;
	unsigned char src_mac[6];
	//unsigned char cur_seq;
	unsigned char need_seq;
	unsigned char base_len;
	unsigned char total_len;
	unsigned char pswd_crc;
	unsigned char recv_len;
	unsigned short seq_success_map;
	unsigned short seq_success_map_cmp;
	union airkiss_data data;
}_airkiss_local_cfg;

const char airkiss_vers[] = "V1.0";

static airkiss_config_t *akconf = 0;
static airkiss_context_t *akcontex = 0;
static _airkiss_local_cfg *air_cfg = 0;

//crc-8

unsigned char calcrc_1byte(unsigned char abyte)    
{    
	unsigned char i,crc_1byte;     
	crc_1byte=0;                
	for(i = 0; i < 8; i++)    
	{    
		if(((crc_1byte^abyte)&0x01))    
		{    

			crc_1byte^=0x18;     
			crc_1byte>>=1;    
			crc_1byte|=0x80;    
		}          
		else    
		{
			crc_1byte>>=1; 
		}
		abyte>>=1;          
	}   
	return crc_1byte;   
}  


unsigned char calcrc_bytes(unsigned char *p,unsigned char len)  
{  
	unsigned char crc=0;  
	while(len--) 
	{  
		crc=calcrc_1byte(crc^*p++);  
	}  
	return crc;   
} 

const char* airkiss_version(void)
{
	return airkiss_vers;
}


static void airkiss_record_move_ones(void *base_addr, int record_num)
{
	int i; 
	unsigned short *record_base = base_addr;

	for(i = 0; i < record_num; i++)
	{
		record_base[i] = record_base[i+1];
	}
}

static void airkiss_add_seq_data(unsigned char *data, int seq)
{
	if(seq < air_cfg->need_seq)
	{
		if((seq*4 + 4) <= USR_DATA_BUFF_MAX_SIZE)
		{
			if((air_cfg->seq_success_map & (1 << seq)) == 0) 
			{
				akconf->memcpy(air_cfg->usr_data + seq*4, data, 4);
				air_cfg->seq_success_map |= 1 << seq;
			}
		}
	}
}

int airkiss_init(airkiss_context_t* context, 
							const airkiss_config_t* config)
{
	if(!context || !config)
		return -1;

	akcontex = context;
	akconf = (airkiss_config_t*)config;
	air_cfg = (_airkiss_local_cfg *)akcontex;

	akconf->memset(air_cfg , 0, sizeof(_airkiss_local_cfg));
	air_cfg->airkiss_state = AIRKISS_STATE_IDLE;

	akconf->printf("air_cfg size:%ld\n", sizeof(_airkiss_local_cfg));
	return 0;
}

static void airkiss_deinit()
{
	akconf = 0;
	akcontex = 0;
	air_cfg = 0;
}

static void resest_airkiss_data()
{
	if(!air_cfg)
		return;
	akconf->memset(&air_cfg->data, 0, sizeof(union airkiss_data));
}

static void airkiss_recv_discover(const void* frame, unsigned short length)
{
	int success = 0;
	if(!air_cfg)
		return;

	if(length > 100)
		return;

	air_cfg->data.guide_code.length_record[MAX_GUIDE_RECORD] = length;

	airkiss_record_move_ones(air_cfg->data.guide_code.length_record, MAX_GUIDE_RECORD);

	 // 1 2 3 4
	if((air_cfg->data.guide_code.length_record[1] - air_cfg->data.guide_code.length_record[0] == 1) &&
	(air_cfg->data.guide_code.length_record[2] - air_cfg->data.guide_code.length_record[1] == 1) &&
	(air_cfg->data.guide_code.length_record[3] - air_cfg->data.guide_code.length_record[2] == 1))
	{
		air_cfg->base_len = air_cfg->data.guide_code.length_record[0] - 1;
		success = 1;
	}
	
	if(success)
	{
		air_cfg->airkiss_state = AIRKISS_STATE_SRC_LOCKED;
		resest_airkiss_data();
		akconf->printf("airkiss_recv_discover success\n");
		akconf->printf("base len:%d\n", air_cfg->base_len);
	}
}


static void airkiss_process_magic_code(airkiss_context_t* context, 
							const void* frame, unsigned short length)
{
	if(!air_cfg)
		return;

	air_cfg->data.magic_code.record[MAX_MAGIC_CODE_RECORD ] = length - air_cfg->base_len;

	airkiss_record_move_ones(air_cfg->data.magic_code.record, MAX_MAGIC_CODE_RECORD);

	if((air_cfg->data.magic_code.record[0]&0xfff0)==0x0000&&
		(air_cfg->data.magic_code.record[1]&0xfff0)==0x0010&&
			(air_cfg->data.magic_code.record[2]&0xfff0)==0x0020&&
			(air_cfg->data.magic_code.record[3]&0xfff0)==0x0030)
	{
		air_cfg->total_len = ((air_cfg->data.magic_code.record[0] & 0x000F) << 4) + (air_cfg->data.magic_code.record[1] & 0x000F);
		air_cfg->ssid_crc = ((air_cfg->data.magic_code.record[2] & 0x000F) << 4) + (air_cfg->data.magic_code.record[3] & 0x000F);
		air_cfg->airkiss_state = AIRKISS_STATE_MAGIC_CODE_COMPLETE;
		resest_airkiss_data();
		akconf->printf("airkiss_process_magic_code success\n");
		akconf->printf("total_len:%d, ssid crc:%x\n", air_cfg->total_len, air_cfg->ssid_crc);
	}
}

static void airkiss_process_prefix_code(airkiss_context_t* context, 
							const void* frame, unsigned short length)
{
	if(!air_cfg)
		return;
	
	air_cfg->data.prefix_code.record[MAX_PREFIX_CODE_RECORD] = length - air_cfg->base_len;

	airkiss_record_move_ones(air_cfg->data.prefix_code.record, MAX_PREFIX_CODE_RECORD );

	if((air_cfg->data.prefix_code.record[0]&0xfff0)==0x0040&&
		(air_cfg->data.prefix_code.record[1]&0xfff0)==0x0050&&
			(air_cfg->data.prefix_code.record[2]&0xfff0)==0x0060&&
			(air_cfg->data.prefix_code.record[3]&0xfff0)==0x0070)
	{
		air_cfg->pswd_len = ((air_cfg->data.prefix_code.record[0] & 0x000F) << 4) + (air_cfg->data.prefix_code.record[1] & 0x000F);
		if(air_cfg->pswd_len > PASSWORD_MAX_LEN)
			air_cfg->pswd_len = 0;

		air_cfg->pswd_crc = ((air_cfg->data.prefix_code.record[2] & 0x000F) << 4) + (air_cfg->data.prefix_code.record[3] & 0x000F);
		air_cfg->airkiss_state = AIRKISS_STATE_PREFIX_CODE_COMPLETE;
		air_cfg->need_seq = ((air_cfg->pswd_len + 1) + 3)/4; //all we need is password and random
		air_cfg->seq_success_map_cmp = (1 << air_cfg->need_seq) - 1; // EXAMPLE: need_seq = 5; seq_success_map_cmp = 0x1f;
			
		resest_airkiss_data();
		akconf->printf("airkiss_process_prefix_code success\n");
		akconf->printf("pswd_len:%d, pswd_crc:%x, need seq:%d, seq map:%x\n", 
                air_cfg->pswd_len, air_cfg->pswd_crc, air_cfg->need_seq, air_cfg->seq_success_map_cmp);
	}
}

static void airkiss_process_sequence(airkiss_context_t* context, 
							const void* frame, unsigned short length)
{
	unsigned char tempBuffer[6];
	
	if(!air_cfg)
		return;
	
	air_cfg->data.seq_code.record[MAX_SEQ_CODE_RECORD] = length - air_cfg->base_len;

	airkiss_record_move_ones(air_cfg->data.seq_code.record, MAX_SEQ_CODE_RECORD);

	if((((air_cfg->data.seq_code.record[0]>>7)&0x0001)==1) &&
		(((air_cfg->data.seq_code.record[1]>>7)&0x0001)==1) && 
		((air_cfg->data.seq_code.record[1]&0x7f) <= ((air_cfg->total_len>>2)+1)))
	{
		tempBuffer[0]=air_cfg->data.seq_code.record[0]&0x7F; 
		tempBuffer[1]=air_cfg->data.seq_code.record[1]&0x7F;
		tempBuffer[2]=air_cfg->data.seq_code.record[2]&0xff;
		tempBuffer[3]=air_cfg->data.seq_code.record[3]&0xff;
		tempBuffer[4]=air_cfg->data.seq_code.record[4]&0xff;
		tempBuffer[5]=air_cfg->data.seq_code.record[5]&0xff;

		akconf->printf("seq:%d, %x,%x,%x,%x\n", tempBuffer[1], tempBuffer[2], tempBuffer[3], tempBuffer[4], tempBuffer[5]);
		if(tempBuffer[0] == (calcrc_bytes(tempBuffer+1,5)&0x7F) )
		{
			int cur_seq = tempBuffer[1];

			airkiss_add_seq_data(&tempBuffer[2], cur_seq);

			akconf->printf("now seq map:%x\n", air_cfg->seq_success_map);
			resest_airkiss_data();

			if(air_cfg->seq_success_map_cmp == air_cfg->seq_success_map)
			{
				air_cfg->random_num = air_cfg->usr_data[air_cfg->pswd_len];
				air_cfg->usr_data[air_cfg->pswd_len] = 0;
				air_cfg->pwd = (char*)air_cfg->usr_data;
				air_cfg->ssid = NULL;
				air_cfg->airkiss_state = AIRKISS_STATE_COMPLETE;
			}
		}
        else
        {
			akconf->printf("CRC error, invalid seq.\n");
		}
	}
}


int airkiss_recv(airkiss_context_t* context, 
							const void* frame, unsigned short length)
{
	//akconf->printf("[airkiss_recv %d]", length);
	if(!air_cfg)
		return AIRKISS_STATUS_CONTINUE;

	switch(air_cfg->airkiss_state)
	{
		case AIRKISS_STATE_IDLE:
			airkiss_recv_discover(frame, length);
			if(air_cfg->airkiss_state == AIRKISS_STATE_SRC_LOCKED)
				return AIRKISS_STATUS_CHANNEL_LOCKED;
			break;
		case AIRKISS_STATE_SRC_LOCKED:
			airkiss_process_magic_code(context, frame, length);
			
			break;
		case AIRKISS_STATE_MAGIC_CODE_COMPLETE:
			airkiss_process_prefix_code(context, frame, length);
			break;	
		case AIRKISS_STATE_PREFIX_CODE_COMPLETE:
			airkiss_process_sequence(context, frame, length);
			if(air_cfg->airkiss_state == AIRKISS_STATE_COMPLETE)
				return AIRKISS_STATUS_COMPLETE;
			break;		
		default:
			air_cfg->airkiss_state = AIRKISS_STATE_IDLE;
			break;
	}

	return AIRKISS_STATUS_CONTINUE;
}


int airkiss_get_result(airkiss_context_t* context, 
							airkiss_result_t* result)
{
	if(!air_cfg || !result)
		return -1;

	memcpy(result, air_cfg, sizeof(airkiss_result_t));

	return 0;
}
