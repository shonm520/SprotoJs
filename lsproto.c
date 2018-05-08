
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "msvcint.h"
#include "cJSON.h"
#include "lsproto.h"

#define MAX_GLOBALSPROTO 16
#define ENCODE_BUFFERSIZE 2050

#define ENCODE_MAXSIZE 0x1000000
#define ENCODE_DEEPLEVEL 64


struct sproto * lnewproto(const char * buffer, size_t sz) {
	struct sproto * sp;
	sp = sproto_create(buffer, sz);
	return sp;
}

int ldeleteproto(struct sproto * sp) {
	sproto_release(sp);
	return 0;
}

struct sproto_type * lquerytype(struct sproto *sp, const char * type_name) {
	if (sp == NULL) {
		return NULL;
	}
	return  sproto_type(sp, type_name);
}

int encode(const struct sproto_arg *args) 
{
	struct encode_ud *self = NULL;
	cJSON* pItem = NULL;
	self = (struct encode_ud *)(args->ud);
	if (self->deep >= ENCODE_DEEPLEVEL)
		return -1;

	if (self->json_item == NULL)
        return -1;
    
    pItem = cJSON_GetObjectItem(self->json_item, args->tagname);
	if (pItem == NULL) {
		return SPROTO_CB_NIL;
	}
	if (args->index > 0)  {
		pItem = cJSON_GetArrayItem(pItem, args->index - 1);
		if (pItem == NULL)  {     //Êý×é½áÊø
			return SPROTO_CB_NIL;
		}
	}
	switch (args->type) {
	case SPROTO_TINTEGER: {
		int64_t v;
        int64_t vh;
        if (pItem->type != cJSON_Number)
        {
            return -1;
        }

		v = (int64_t)(pItem->valueint);

		vh = v >> 31;
		if (vh == 0 || vh == -1) {
			*(uint32_t *)args->value = (uint32_t)v;
			return 4;
		}
		else {
			*(uint64_t *)args->value = (uint64_t)v;
			return 8;
		}
	}
	case SPROTO_TBOOLEAN: {
		int v = 0;
        if (pItem->type != cJSON_False && pItem->type != cJSON_True){
            return -1;
        }

		v = 0;
        if (pItem->type != cJSON_True){
            v = 1;
        }
		*(int *)args->value = v;
		return 4;
	}
	case SPROTO_TSTRING: {
		size_t sz = 0;
        if (pItem->type != cJSON_String){
            return -1;
        }
		sz = strlen(pItem->valuestring);
		if (sz > args->length)
			return -1;
		memcpy(args->value, pItem->valuestring, sz);
		return sz + 1;
	}
	case SPROTO_TSTRUCT: {
		struct encode_ud sub;
		int r = 0;
        if (pItem->type != cJSON_Array && pItem->type != cJSON_Object){
            return -1;
        }

		sub.st = args->subtype;
		sub.json_item = pItem;

        r = sproto_encode(args->subtype, args->value, args->length, encode, &sub);
		if (r < 0) 
			return r;
		return r;
	}
	default:
		return -1;
	}
}

void * expand_buffer(void* p, int osz, int nsz) {
	void *output;
	do {
		osz *= 2;
	} while (osz < nsz);
	if (osz > ENCODE_MAXSIZE) {
		return NULL;
	}
	output  = realloc(p, osz);
	return output;
}

/*
	lightuserdata sproto_type
	table source

	return string
 */
int lencode(struct sproto * sp,struct sproto_type * st,  char* jsonStr, char** ppBuffer, size_t sz) {
	struct encode_ud self;
	cJSON *json = NULL;
	self.st = st;
	self.tbl_index = 0;
    json = cJSON_Parse(jsonStr);
    if (json == NULL)
        return 0;

	for (;;) {
		int r = 0;
		self.array_tag = NULL;
		self.array_index = 0;
		self.deep = 0;
		self.iter_index = self.tbl_index+1;
        self.json_item = json;
		r = sproto_encode(st, *ppBuffer, sz, encode, &self);
		if (r<0) {
			*ppBuffer = (char*)expand_buffer(*ppBuffer, sz, sz*2);
			sz *= 2;
		} else {
			return 1;
		}
	}
    return 0;
}

int
decode(const struct sproto_arg *args) {
	struct decode_ud * self = (decode_ud*)args->ud;
	if (self->deep >= ENCODE_DEEPLEVEL)
		return -1;
	if (args->length == 0)  {
		return 0;
	}
	cJSON* arr_js = NULL;
	cJSON* item = NULL;
	if (args->index > 0) {
		// It's array
		if (args->tagname != self->array_tag) {
			self->array_tag = args->tagname;
			arr_js = cJSON_CreateArray();
			cJSON_AddItemToObject(self->json_item, args->tagname, arr_js);			
		}
		else  {
			arr_js = cJSON_GetObjectItem(self->json_item, args->tagname);
		}
	}
	switch (args->type) {
	case SPROTO_TINTEGER: {
		// notice: in lua 5.2, 52bit integer support (not 64)
		int val = *(uint64_t*)args->value;
		item = cJSON_CreateNumber(val);
		break;
	}
	case SPROTO_TBOOLEAN: {
		int v = *(uint64_t*)args->value;
		break;
	}
	case SPROTO_TSTRING: {
		char* val = (char*)args->value;
		char* str = new char[args->length + 1];
		strncpy(str, val, args->length);
		str[args->length] = 0;
		item = cJSON_CreateString(str);
		break;
	}
	case SPROTO_TSTRUCT: {
		struct decode_ud sub;
		int r;
		sub.deep = self->deep + 1;
		sub.array_index = 0;
		sub.array_tag = NULL;
		item = cJSON_CreateObject();
		sub.json_item = item;
		r = sproto_decode(args->subtype, args->value, args->length, decode, &sub);
		if (r < 0 || r != args->length)
			return r;
		break;
	}
	default:;
		//luaL_error(L, "Invalid type");
	}
	if (args->index > 0)  {
		if (arr_js && item)
			cJSON_AddItemToArray(arr_js, item);
	}
	else  {
		if (item)
			cJSON_AddItemToObject(self->json_item, args->tagname, item);
	}
	return 0;
}

void ldumpproto(struct sproto * sp) {
	sproto_dump(sp);
}

/*
	string source	/  (lightuserdata , integer)
	return string
 */
char* lpack(const void * buffer, size_t sz, size_t* pOutSize) {
	// the worst-case space overhead of packing is 2 bytes per 2 KiB of input (256 words = 2KiB).
	int bytes;
	size_t maxsz = (sz + 2047) / 2048 * 2 + sz + 2;
	void * output = malloc(10*1024);
	int osz = 10*1024;
	if (osz < maxsz) {
		output = expand_buffer(output, osz, maxsz);
	}
	bytes = sproto_pack(buffer, sz, output, maxsz);
	if (bytes > maxsz) {
		return NULL;
	}
	return (char*)output;
}

char*  lunpack(const void * buffer, size_t sz, size_t* pOutSize) {
	char * output = (char*)malloc(10*1024);
	int osz =10*1024;
	int r = sproto_unpack(buffer, sz, output, osz);
	if (r < 0)
		return NULL;
	if (r > osz) {
		output = (char*)expand_buffer(output, osz, r);
		r = sproto_unpack(buffer, sz, output, r);
		if (r < 0)
			return NULL;
	}
    *pOutSize = osz;
	return output;
}

int lprotocol(struct sproto * sp, const char* name,  struct protocol_ret * pRet) {
    struct protocol_ret;
    int tag;

    tag = sproto_prototag(sp, name);
    if (tag < 0)
        return 0;

    pRet->tag = tag;

    pRet->request = sproto_protoquery(sp, tag, SPROTO_REQUEST);

    pRet->response = sproto_protoquery(sp, tag, SPROTO_RESPONSE);

    return 3;
}

/* global sproto pointer for multi states
   NOTICE : It is not thread safe
 */
 struct sproto * G_sproto[MAX_GLOBALSPROTO];

int lsaveproto(struct sproto * sp, int index) {
	if (index < 0 || index >= MAX_GLOBALSPROTO) {
		return 0;
	}
	G_sproto[index] = sp;
	return 1;
}

struct sproto * lloadproto(int index) {
	struct sproto * sp;
	if (index < 0 || index >= MAX_GLOBALSPROTO) {
		return NULL;
	}
	sp = G_sproto[index];
	if (sp == NULL) {
		return NULL;
	}
    return sp;
}

#if 0
 void
push_default(const struct sproto_arg *args, int array) {
	lua_State *L = args->ud;
	switch(args->type) {
	case SPROTO_TINTEGER:
		if (args->extra)
			lua_pushnumber(L, 0.0);
		else
			lua_pushinteger(L, 0);
		break;
	case SPROTO_TBOOLEAN:
		lua_pushboolean(L, 0);
		break;
	case SPROTO_TSTRING:
		lua_pushliteral(L, "");
		break;
	case SPROTO_TSTRUCT:
		if (array) {
			lua_pushstring(L, sproto_name(args->subtype));
		} else {
			lua_createtable(L, 0, 1);
			lua_pushstring(L, sproto_name(args->subtype));
			lua_setfield(L, -2, "__type");
		}
		break;
	default:
		luaL_error(L, "Invalid type %d", args->type);
		break;
	}
}

 int
encode_default(const struct sproto_arg *args) {
	lua_State *L = args->ud;
	lua_pushstring(L, args->tagname);
	if (args->index > 0) {
		lua_newtable(L);
		push_default(args, 1);
		lua_setfield(L, -2, "__array");
		lua_rawset(L, -3);
		return SPROTO_CB_NOARRAY;
	} else {
		push_default(args, 0);
		lua_rawset(L, -3);
		return SPROTO_CB_NIL;
	}
}

/*
	lightuserdata sproto_type
	return default table
 */
 int
ldefault(lua_State *L) {
	int ret;
	// 64 is always enough for dummy buffer, except the type has many fields ( > 27).
	char dummy[64];
	struct sproto_type * st = lua_touserdata(L, 1);
	if (st == NULL) {
		return luaL_argerror(L, 1, "Need a sproto_type object");
	}
	lua_newtable(L);
	ret = sproto_encode(st, dummy, sizeof(dummy), encode_default, L);
	if (ret<0) {
		// try again
		int sz = sizeof(dummy) * 2;
		void * tmp = lua_newuserdata(L, sz);
		lua_insert(L, -2);
		for (;;) {
			ret = sproto_encode(st, tmp, sz, encode_default, L);
			if (ret >= 0)
				break;
			sz *= 2;
			tmp = lua_newuserdata(L, sz);
			lua_replace(L, -3);
		}
	}
	return 1;
}

	luaL_Reg l[] = {
		{ "newproto", lnewproto },
		{ "deleteproto", ldeleteproto },
		{ "dumpproto", ldumpproto },
		{ "querytype", lquerytype },
		{ "decode", ldecode },
		{ "protocol", lprotocol },
		{ "loadproto", lloadproto },
		{ "saveproto", lsaveproto },
		{ "default", ldefault },
		{ NULL, NULL },
	};
	luaL_newlib(L,l);
	pushfunction_withbuffer(L, "encode", lencode);
	pushfunction_withbuffer(L, "pack", lpack);
	pushfunction_withbuffer(L, "unpack", lunpack);

#endif

