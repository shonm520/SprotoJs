
#ifndef __csproto_h__
#define __csproto_h__
#include "sproto.h"
#include "lsproto.h"

#ifdef SPROTOJS_EXPORTS
#define EX_PORT __declspec(dllexport)
#else
#define EX_PORT __declspec(dllimport)
#endif

class EX_PORT csproto{
	struct sproto * _sp;

private:
	struct sproto_type* querytype(const char* typeName);

	int encode(const char* typeName, const char* json, char** ppOutBuffer, int * pSize);

	int decode(const char* typeName, const char* buffer, int sz, char** ppOutBuffer, int * pSize);

	int encode_item(const struct sproto_type *, char* pBuffer, int size, const char* json);

	char* decode_item(const struct sproto_type *, const void *intBuffer, int sz, int* paresedSz);


	int pack(const char* buffer, int sz, char** ppOutBuffer, int * pSize);

	int unpack(const char* buffer, int sz, char** ppOutBuffer, int * pSize);

public:
	int new_spro(const char* protobin, int sz);

	char* dispatch(const void *intBuffer, int sz, char** ppOutBuffer, int *pSize);

	char* request(const char* name, const char* json, char** ppBuffer, int size, int* ret);
};

#endif
