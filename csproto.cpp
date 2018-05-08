#include <stdio.h>
#include "cJSON.h"
#include "lsproto.h"
#include "csproto.h"

int csproto::new_spro(const char* protobin, int sz)
{
	_sp = lnewproto(protobin, sz);
	if (_sp == NULL)
		return -1;
	return 0;
}

char* csproto::dispatch(const void *intBuffer, int sz, char** ppOutBuffer, int *pSize)
{
	char* unPackedBytes = new char[2050];
	sz = sproto_unpack(intBuffer, sz, unPackedBytes, 2050);

	struct sproto_type* st = sproto_type(_sp, "package");
	int headSz = 0;
	char* js = decode_item(st, unPackedBytes, sz, &headSz);
	cJSON* json = cJSON_Parse(js);
	cJSON* pItem = cJSON_GetObjectItem(json, "type");
	int tag = pItem->valueint;

	cJSON_Delete(json);
	delete[] js;

	struct sproto_type* request = sproto_protoquery(_sp, tag, SPROTO_REQUEST); 
	js = decode_item(request, unPackedBytes + headSz, sz - headSz, NULL);

	delete[] unPackedBytes;

	return js;
}

char* csproto::request(const char* name, const char* json, char** ppBuffer, int size, int* ret)
{
	struct sproto_type* st = sproto_type(_sp, "package");
	if (st == NULL)  {
		return NULL;
	}
	int tag = sproto_prototag(_sp, name);
	char pNameJson[100] = {0};
	sprintf_s(pNameJson, 100, "{\"type\":%d}", tag);
	int num_head = encode_item(st, *ppBuffer, size, pNameJson);     //先打包头
	if (num_head == -1)  {
		return NULL;
	}

	char szRequst[100] = { 0 };
	strcpy_s(szRequst, name);
	strcat_s(szRequst, ".request");
	st = sproto_type(_sp, szRequst);
	if (st == NULL)  {
		return NULL;
	}

	int num_body = encode_item(st, *ppBuffer + num_head, size - num_head, json);
	if (num_body == -1)  {
		return NULL;
	}

	int num_total = num_head + num_body;
	char* pTotal = new char[num_total];

	memcpy(pTotal, *ppBuffer, num_total);

	*ret = num_total;

	*ret = sproto_pack(pTotal, *ret, *ppBuffer, *ret);
	delete[] pTotal;

	return *ppBuffer;
}

//-------------------------   private  -----------------------------

int csproto::encode_item(const struct sproto_type* st, char* ppBuffer, int size, const char* json)
{
	if (ppBuffer == NULL)  {
		return -1;
	}
	struct encode_ud self = {0};
	self.json_item = cJSON_Parse(json);
	if (self.json_item == NULL)  {
		return -1;
	}
	memset(ppBuffer, 0, size);
	int r = sproto_encode(st, ppBuffer, size, ::encode, &self);      //ppBuffer只作传出用
	cJSON_Delete(self.json_item);
	return (r < 0) ? -1 : r;
}

char* csproto::decode_item(const struct sproto_type* st, const void *intBuffer, int sz, int* paresedSz)
{
	struct decode_ud self = { 0 };
	cJSON* js = cJSON_CreateObject();
	self.json_item = js;
	int num = sproto_decode(st, intBuffer, sz, ::decode, &self);
	if (paresedSz)  {
		*paresedSz = num;
	}
	char* pJs = cJSON_Print(js);
	int len = strlen(pJs);
	char* rJs = new char[len + 1];
	strncpy(rJs, pJs, len);
	rJs[len] = 0;
	cJSON_Delete(js);
	return rJs;
}

struct sproto_type* csproto::querytype(const char* typeName)
{
	//要加缓存
	int tag = sproto_prototag(_sp, typeName);
	struct sproto_type* request = sproto_protoquery(_sp, tag, SPROTO_REQUEST);
	return request;
}

int csproto::encode(const char* typeName, const char* json, char** ppOutBuffer, int * pSize)
{
    return 0;
}

int csproto::decode(const char* typeName, const char* buffer, int sz, char** ppOutBuffer, int * pSize)
{
    return 0;
}

int csproto::pack(const char* buffer, int sz, char** ppOutBuffer, int * pSize)
{
    return 0;
}

int csproto::unpack(const char* buffer, int sz, char** ppOutBuffer, int * pSize)
{
    return 0;
}

