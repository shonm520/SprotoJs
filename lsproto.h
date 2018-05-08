#ifndef __lsproto_h__
#define __lsproto_h__

#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "msvcint.h"
#include "sproto.h"

#ifdef __cplusplus
extern "C" {
#endif

struct encode_ud{
	struct sproto_type *st;
	int tbl_index;
	const char * array_tag;
	struct cJSON *   json_item;
	int array_index;
	int deep;
	int iter_index;
};

struct decode_ud{
	const char * array_tag;
	int array_index;
	int result_index;
	int deep;
	int mainindex_tag;
	int key_index;
	struct cJSON* json_item;
};

struct protocol_ret{
	int tag;
	struct sproto_type *request;
	struct sproto_type *response;
};

struct sproto * lnewproto(const char * buffer, size_t sz);

int ldeleteproto(struct sproto * sp) ;

struct sproto_type * lquerytype(struct sproto *sp, const char * type_name);

int encode(const struct sproto_arg *args);

void * expand_buffer(void* p, int osz, int nsz);

int lencode(struct sproto * sp,struct sproto_type * st,  char* jsonStr, char** ppBuffer, size_t sz);

int decode(const struct sproto_arg *args);

void ldumpproto(struct sproto * sp);

char* lpack(const void * buffer, size_t sz, size_t* pOutSize);

char*  lunpack(const void * buffer, size_t sz, size_t* pOutSize);

int lprotocol(struct sproto * sp, const char* name,  struct protocol_ret * pRet);

int lsaveproto(struct sproto * sp, int index);

struct sproto * lloadproto(int index);
	
#ifdef __cplusplus
}
#endif
#endif