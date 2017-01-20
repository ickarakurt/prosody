/* Prosody IM
-- Copyright (C) 2008-2016 Matthew Wild
-- Copyright (C) 2008-2016 Waqas Hussain
-- Copyright (C) 2016 Kim Alvefur
--
-- This project is MIT/X11 licensed. Please see the
-- COPYING file in the source package for more information.
--
*/

/*
* crand.c
* C PRNG interface
*/

#include "lualib.h"
#include "lauxlib.h"

#include <string.h>
#include <errno.h>

#if defined(WITH_GETRANDOM)
#include <unistd.h>
#include <sys/syscall.h>
#include <linux/random.h>

#ifndef SYS_getrandom
#error getrandom() requires Linux 3.17 or later
#endif

/* Was this not supposed to be a function? */
int getrandom(void *buf, size_t len, int flags) {
	return syscall(SYS_getrandom, buf, len, flags);
}

#elif defined(WITH_ARC4RANDOM)
#include <stdlib.h>
#elif defined(WITH_OPENSSL)
#include <openssl/rand.h>
#else
#error util.crand compiled without a random source
#endif

int Lrandom(lua_State *L) {
	int ret = 0;
	size_t len = (size_t)luaL_checkinteger(L, 1);
	void *buf = lua_newuserdata(L, len);

#if defined(WITH_GETRANDOM)
	ret = getrandom(buf, len, 0);

	if(ret < 0) {
		lua_pushnil(L);
		lua_pushstring(L, strerror(errno));
		lua_pushinteger(L, errno);
		return 3;
	}

#elif defined(WITH_ARC4RANDOM)
	arc4random_buf(buf, len);
	ret = len;
#elif defined(WITH_OPENSSL)
	ret = RAND_bytes(buf, len);

	if(ret == 1) {
		ret = len;
	} else {
		lua_pushnil(L);
		lua_pushstring(L, "failed");
		/* lua_pushinteger(L, ERR_get_error()); */
		return 2;
	}

#endif

	lua_pushlstring(L, buf, ret);
	return 1;
}

int luaopen_util_crand(lua_State *L) {
#if (LUA_VERSION_NUM > 501)
	luaL_checkversion(L);
#endif
	lua_newtable(L);
	lua_pushcfunction(L, Lrandom);
	lua_setfield(L, -2, "bytes");

#if defined(WITH_GETRANDOM)
	lua_pushstring(L, "Linux");
#elif defined(WITH_ARC4RANDOM)
	lua_pushstring(L, "arc4random()");
#elif defined(WITH_OPENSSL)
	lua_pushstring(L, "OpenSSL");
#endif
	lua_setfield(L, -2, "_source");

#if defined(WITH_OPENSSL) && defined(_WIN32)
	/* Do we need to seed this on Windows? */
#endif

	return 1;
}

