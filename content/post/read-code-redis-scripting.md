+++
title = "代码阅读: redis-scripting"
date = "2018-01-13"
categories = ["Linux"]
tags = ["read-code", "c", "redis"]
slug = "read-code-redis-scripting"
Authors = "Xudong"
+++

这里是lua支持相关的部分，也就是eval.
ldb是调试相关的部分，先都忽略掉.
lua的简单背景，数组是下标从1开始，栈也是，1表示栈底，-1是栈顶.


#### char *redisProtocolToLuaType_Int(lua_State *lua, char *reply)
将redis的类型转换成lua的int类型，并推到栈顶.

    // p指向从reply+1开始的第一个\r.
    char *p = strchr(reply+1,'\r');
    long long value;
    // 将reply+1到p间的字符串转换成long long然后转换成lua_Number并入栈
    string2ll(reply+1,p-reply-1,&value);
    lua_pushnumber(lua,(lua_Number)value);
    // 跳过\r和\n两个字符
    return p+2;

#### char *redisProtocolToLuaType_Status(lua_State *lua, char *reply)
将reply+1到\r直接的字符串变成一个{ok: STRING}的lua table，并入栈

    char *p = strchr(reply+1,'\r');
    // 初始化一个table并push到栈顶
    lua_newtable(lua);
    // push ok这个字符串到栈顶，再push reply+1后面的字符串
    lua_pushstring(lua,"ok");
    lua_pushlstring(lua,reply+1,p-reply-1);
    // 此时栈里数据大致是这样的[..., TABLE, 'ok', REDIS_STRING]
    // 下标-3对应的是TABLE, 也就是之前lua_newtable初始化的那个
    // lua_settable(lua, -3) => TABLE['ok'] = REDIS_STRING
    // 同时还会pop key, value，也就是table到栈顶了
    lua_settable(lua,-3);
    return p+2;

#### char *redisProtocolToLuaType_MultiBulk(lua_State *lua, char *reply)
先获取后面的数据类型的个数n，然后获取后面的n个类型依次放入一个array里，并入栈.

    char *p = strchr(reply+1,'\r');
    long long mbulklen;
    int j = 0;
    string2ll(reply+1,p-reply-1,&mbulklen);
    p += 2;
    if (mbulklen == -1) {
        lua_pushboolean(lua,0);
        return p;
    }
    lua_newtable(lua);
    for (j = 0; j < mbulklen; j++) {
        // table的下标是从1开始的int(也就是array类型)
        lua_pushnumber(lua,j+1);
        p = redisProtocolToLuaType(lua,p);
        lua_settable(lua,-3);
    }
    return p;

#### char *redisProtocolToLuaType(lua_State *lua, char* reply)
根据第一个字符来确定不同的类型，调用不同方法处理(这里前缀字符是在networking.c里面添加的)

    char *p = reply;
    switch(*p) {
    case ':': p = redisProtocolToLuaType_Int(lua,reply); break;
    case '$': p = redisProtocolToLuaType_Bulk(lua,reply); break;
    case '+': p = redisProtocolToLuaType_Status(lua,reply); break;
    case '-': p = redisProtocolToLuaType_Error(lua,reply); break;
    case '*': p = redisProtocolToLuaType_MultiBulk(lua,reply); break;
    }

#### void luaPushError(lua_State *lua, char *error)
将error字符串转换成{err: ERROR}这样的map, 然后推到栈顶.

    lua_newtable(lua);
    lua_pushstring(lua,"err");
    lua_pushstring(lua, error);
    lua_settable(lua,-3);

#### int luaRaiseError(lua_State *lua)
获取luaPushError推入的error字符串，产生lua错误.

    lua_pushstring(lua,"err");
    // 栈结构[error_table, "err"], lua_gettable之后变成了[error_table, error_string]
    lua_gettable(lua,-2);
    return lua_error(lua);

#### void luaSortArray(lua_State *lua)
对栈顶的array进行排序，首先使用lua自身的table.sort方法，如果出错了，就使用自定义的比较函数，也就是执行table.sort(ARRAY, cmp_func)

    // 将table入栈，[ARRAY, table]
    lua_getglobal(lua,"table");
    // 将"sort"字符串入栈, [ARRAY, table, "sort"]
    lua_pushstring(lua,"sort");
    // -2下标对应的是第一行压进的table, 这里获取table.sort方法
    // 同时"sort"字符串出栈, table.sort入栈, [ARRAY, table, table.sort]
    lua_gettable(lua,-2);
    // 将下标-3(也就是ARRAY)复制出来并推到栈顶
    // 栈结构变成了[ARRAY, table, table.sort, ARRAY]
    lua_pushvalue(lua,-3);
    // lua_pcall执行函数，只有一个参数，没有返回值，也就是执行了
    // table.sort(ARRAY), 返回值0是成功，其他是有错误
    if (lua_pcall(lua,1,0,0)) {
        // 如果进入if里面说明执行出错了, 栈为[ARRAY, table, ERROR]
        // 将ERROR出栈，然后重新组装函数和参数
        lua_pop(lua,1);
        lua_pushstring(lua,"sort");
        lua_gettable(lua,-2);
        lua_pushvalue(lua,-3);
        // 这里再压入一个比较大小的helper函数，作为table.sort的第二个参数
        // 这个函数是一开始初始化的定义的一个lua函数
        // 此时栈为 [ARRAY, table, table.sort, ARRAY, __redis__compare_helper]
        lua_getglobal(lua,"__redis__compare_helper");
        // 这里直接lua_call不捕获错误了, 2是因为函数有两个参数
        lua_call(lua,2,0);
    }
    // 上面执行完之后栈为[ARRAY, table], 将table给pop掉，这样sort好了的array就在栈顶了
    lua_pop(lua,1);

#### void luaReplyToRedisReply(client *c, lua_State *lua)
将lua的返回值变成redis的返回结果, 重点看LUA_TABLE的处理(string, bool的删除了)

    // 获取栈顶(lua结果)的数据类型
    int t = lua_type(lua,-1);
    switch(t) {
    case LUA_TNUMBER:
        addReplyLongLong(c,(long long)lua_tonumber(lua,-1));
        break;
    case LUA_TTABLE:
        // 对于table, 有三种可能, 分别是error map, ok map, array
        // 首先检测是不是error结果
        lua_pushstring(lua,"err");
        lua_gettable(lua,-2);
        t = lua_type(lua,-1);
        if (t == LUA_TSTRING) {
            // 如果"err"对于的value是字符串，说明是一个error结果
            // 转换成c字符串, 将"\r\n"替换成space, 然后包装成特定格式返回(-开头表示ERROR, \r\n结尾)
            sds err = sdsnew(lua_tostring(lua,-1));
            sdsmapchars(err,"\r\n","  ",2);
            addReplySds(c,sdscatprintf(sdsempty(),"-%s\r\n",err));
            sdsfree(err);
            // error字符串和table一起pop
            lua_pop(lua,2);
            return;
        }
        // 到这里说明不是error, 需要将栈顶的TABLE["err"]出栈
        lua_pop(lua,1);
        // 后面假设是status reply, 跟error处理几乎一样
        lua_pushstring(lua,"ok");
        lua_gettable(lua,-2);
        t = lua_type(lua,-1);
        if (t == LUA_TSTRING) {
            sds ok = sdsnew(lua_tostring(lua,-1));
            sdsmapchars(ok,"\r\n","  ",2);
            addReplySds(c,sdscatprintf(sdsempty(),"+%s\r\n",ok));
            sdsfree(ok);
            lua_pop(lua,1);
        } else {
            // 到这里table就只能是array了
            void *replylen = addDeferredMultiBulkLength(c);
            int j = 1, mbulklen = 0;
            // 将TABLE["ok"]这个给pop掉
            lua_pop(lua,1);
            while(1) {
                // 从下标1开始一个个的获取array元素，直到类型是nil
                lua_pushnumber(lua,j++);
                lua_gettable(lua,-2);
                t = lua_type(lua,-1);
                if (t == LUA_TNIL) {
                    lua_pop(lua,1);
                    break;
                }
                // 递归调用自身来处理每一个元素
                luaReplyToRedisReply(c, lua);
                mbulklen++;
            }
            setDeferredMultiBulkLength(c,replylen,mbulklen);
        }
        break;
    default:
        addReply(c,shared.nullbulk);
    }
    lua_pop(lua,1);

#### int luaRedisGenericCommand(lua_State *lua, int raise_error)
lua command的处理, raise_error为true就返回一个luaError

    int j, argc = lua_gettop(lua);
    struct redisCommand *cmd;
    client *c = server.lua_client;
    sds reply;

    /* Cached across calls. */
    static robj **argv = NULL;
    static int argv_size = 0;
    static robj *cached_objects[LUA_CMD_OBJCACHE_SIZE];
    static size_t cached_objects_len[LUA_CMD_OBJCACHE_SIZE];
    // 递归检测来使用的(这个函数不允许在里面调用自身)
    static int inuse = 0;

    // 如果出现了递归调用就打错误日志并退出
    if (inuse) {
            // DEAL_WITH_ERROR AND RETURN
    }
    inuse++;

    // 至少需要一个参数
    if (argc == 0) {
            // DEAL_WITH_ERROR AND RETURN
    }

    if (argv_size < argc) {
        argv = zrealloc(argv,sizeof(robj*)*argc);
        argv_size = argc;
    }

    for (j = 0; j < argc; j++) {
        char *obj_s;
        size_t obj_len;
        char dbuf[64];

        if (lua_type(lua,j+1) == LUA_TNUMBER) {
            // 这里没有直接用lua_tostring是因为这个lua api的实现会有精度损失(我没有验证)
            // 这里的作为是线转换到C然后到double，再到string
            lua_Number num = lua_tonumber(lua,j+1);

            obj_len = snprintf(dbuf,sizeof(dbuf),"%.17g",(double)num);
            obj_s = dbuf;
        } else {
            obj_s = (char*)lua_tolstring(lua,j+1,&obj_len);
            // 如果不是字符串就退出
            if (obj_s == NULL) break;
        }

        // 这里对于少于32个参数的情况下，就使用预先定义的静态robj*数组
        // 不用malloc内存，相当于有个小的内存池
        if (j < LUA_CMD_OBJCACHE_SIZE && cached_objects[j] &&
            cached_objects_len[j] >= obj_len)
        {
            sds s = cached_objects[j]->ptr;
            argv[j] = cached_objects[j];
            cached_objects[j] = NULL;
            memcpy(s,obj_s,obj_len+1);
            sdssetlen(s, obj_len);
        } else {
            argv[j] = createStringObject(obj_s, obj_len);
        }
    }

    // 如果j != argc, 说明提前break了，也就是某个参数不是int或者string
    if (j != argc) {
        j--;
        // 减少引用计数
        while (j >= 0) {
            decrRefCount(argv[j]);
            j--;
        }
        // DEAL_WITH_ERROR AND RETURN
    }

    // 构造一个假的client
    c->argv = argv;
    c->argc = argc;

    // 根据名字查找命令函数, 检测参数个数是否符合要求
    cmd = lookupCommand(argv[0]->ptr);
    if (!cmd || ((cmd->arity > 0 && cmd->arity != argc) ||
                   (argc < -cmd->arity)))
    {
        // DEAL_WITH_ERROR
        goto cleanup;
    }
    c->cmd = c->lastcmd = cmd;

    // 有些命令是不允许在lua脚本里执行
    if (cmd->flags & CMD_NOSCRIPT) {
        luaPushError(lua, "This Redis command is not allowed from scripts");
        goto cleanup;
    }

    // 处理有写操作的命令
    if (cmd->flags & CMD_WRITE) {
        if (server.lua_random_dirty && !server.lua_replicate_commands) {
            luaPushError(lua,
                "Write commands not allowed after non deterministic commands. Call redis.replicate_commands() at the start of your script in order to switch to single commands replication mode.");
            goto cleanup;
        } else if (server.masterhost && server.repl_slave_ro &&
                   !server.loading &&
                   !(server.lua_caller->flags & CLIENT_MASTER))
        {
            luaPushError(lua, shared.roslaveerr->ptr);
            goto cleanup;
        } else if (server.stop_writes_on_bgsave_err &&
                   server.saveparamslen > 0 &&
                   server.lastbgsave_status == C_ERR)
        {
            luaPushError(lua, shared.bgsaveerr->ptr);
            goto cleanup;
        }
    }

    /* If we reached the memory limit configured via maxmemory, commands that
     * could enlarge the memory usage are not allowed, but only if this is the
     * first write in the context of this script, otherwise we can't stop
     * in the middle. */
    if (server.maxmemory && server.lua_write_dirty == 0 &&
        (cmd->flags & CMD_DENYOOM))
    {
        if (freeMemoryIfNeeded() == C_ERR) {
            luaPushError(lua, shared.oomerr->ptr);
            goto cleanup;
        }
    }

    if (cmd->flags & CMD_RANDOM) server.lua_random_dirty = 1;
    if (cmd->flags & CMD_WRITE) server.lua_write_dirty = 1;

    /* If this is a Redis Cluster node, we need to make sure Lua is not
     * trying to access non-local keys, with the exception of commands
     * received from our master or when loading the AOF back in memory. */
    if (server.cluster_enabled && !server.loading &&
        !(server.lua_caller->flags & CLIENT_MASTER))
    {
        /* Duplicate relevant flags in the lua client. */
        c->flags &= ~(CLIENT_READONLY|CLIENT_ASKING);
        c->flags |= server.lua_caller->flags & (CLIENT_READONLY|CLIENT_ASKING);
        if (getNodeByQuery(c,c->cmd,c->argv,c->argc,NULL,NULL) !=
                           server.cluster->myself)
        {
            luaPushError(lua,
                "Lua script attempted to access a non local key in a "
                "cluster node");
            goto cleanup;
        }
    }

    /* If we are using single commands replication, we need to wrap what
     * we propagate into a MULTI/EXEC block, so that it will be atomic like
     * a Lua script in the context of AOF and slaves. */
    if (server.lua_replicate_commands &&
        !server.lua_multi_emitted &&
        server.lua_write_dirty &&
        server.lua_repl != PROPAGATE_NONE)
    {
        execCommandPropagateMulti(server.lua_caller);
        server.lua_multi_emitted = 1;
    }

    /* Run the command */
    int call_flags = CMD_CALL_SLOWLOG | CMD_CALL_STATS;
    if (server.lua_replicate_commands) {
        /* Set flags according to redis.set_repl() settings. */
        if (server.lua_repl & PROPAGATE_AOF)
            call_flags |= CMD_CALL_PROPAGATE_AOF;
        if (server.lua_repl & PROPAGATE_REPL)
            call_flags |= CMD_CALL_PROPAGATE_REPL;
    }
    call(c,call_flags);

    /* Convert the result of the Redis command into a suitable Lua type.
     * The first thing we need is to create a single string from the client
     * output buffers. */
    if (listLength(c->reply) == 0 && c->bufpos < PROTO_REPLY_CHUNK_BYTES) {
        /* This is a fast path for the common case of a reply inside the
         * client static buffer. Don't create an SDS string but just use
         * the client buffer directly. */
        c->buf[c->bufpos] = '\0';
        reply = c->buf;
        c->bufpos = 0;
    } else {
        reply = sdsnewlen(c->buf,c->bufpos);
        c->bufpos = 0;
        while(listLength(c->reply)) {
            sds o = listNodeValue(listFirst(c->reply));

            reply = sdscatsds(reply,o);
            listDelNode(c->reply,listFirst(c->reply));
        }
    }
    if (raise_error && reply[0] != '-') raise_error = 0;
    redisProtocolToLuaType(lua,reply);

    /* Sort the output array if needed, assuming it is a non-null multi bulk
     * reply as expected. */
    if ((cmd->flags & CMD_SORT_FOR_SCRIPT) &&
        (server.lua_replicate_commands == 0) &&
        (reply[0] == '*' && reply[1] != '-')) {
            luaSortArray(lua);
    }
    if (reply != c->buf) sdsfree(reply);
    c->reply_bytes = 0;

#### redis.call, redis.pcall的实现

    int luaRedisCallCommand(lua_State *lua) {
        return luaRedisGenericCommand(lua,1);
    }
    int luaRedisPCallCommand(lua_State *lua) {
        return luaRedisGenericCommand(lua,0);
    }

#### void luaLoadLib(lua_State *lua, const char *libname, lua_CFunction luafunc)
载入一个lua模块(官方的或者第三方的)

  lua_pushcfunction(lua, luafunc);
  lua_pushstring(lua, libname);
  lua_call(lua, 1, 0);

#### void luaLoadLibraries(lua_State *lua)
导入所有需要的库，包括几个第三方的库

    luaLoadLib(lua, "", luaopen_base);
    luaLoadLib(lua, LUA_TABLIBNAME, luaopen_table);
    luaLoadLib(lua, LUA_STRLIBNAME, luaopen_string);
    luaLoadLib(lua, LUA_MATHLIBNAME, luaopen_math);
    luaLoadLib(lua, LUA_DBLIBNAME, luaopen_debug);
    luaLoadLib(lua, "cjson", luaopen_cjson);
    luaLoadLib(lua, "struct", luaopen_struct);
    luaLoadLib(lua, "cmsgpack", luaopen_cmsgpack);
    luaLoadLib(lua, "bit", luaopen_bit);

#### void luaRemoveUnsupportedFunctions(lua_State *lua)
删除一些不想暴露出来的方法(将loadfile, dofile设置为nil, 这样就没发调用了)

    lua_pushnil(lua);
    lua_setglobal(lua,"loadfile");
    lua_pushnil(lua);
    lua_setglobal(lua,"dofile");


#### void scriptingEnableGlobalsProtection(lua_State *lua)
载入一段lua脚本，没有仔细研究，__newindex, __index应该是跟python的__setattr__, __getattr__类似的方法.


#### void scriptingInit(int setup)
lua初始化过程，包括实例化，导入需要的库，导入上面的那些c函数

    // 初始化一个lua实例
    lua_State *lua = lua_open();

    if (setup) {
        server.lua_client = NULL;
        server.lua_caller = NULL;
        server.lua_timedout = 0;
        server.lua_always_replicate_commands = 0;   // debug
        server.lua_time_limit = LUA_SCRIPT_TIME_LIMIT;
        ldbInit();                                  // debug
    }
    // 初始化库以及屏蔽不想暴露的函数
    luaLoadLibraries(lua);
    luaRemoveUnsupportedFunctions(lua);

    /* Initialize a dictionary we use to map SHAs to scripts.
     * This is useful for replication, as we need to replicate EVALSHA
     * as EVAL, so we need to remember the associated script. */
    server.lua_scripts = dictCreate(&shaScriptObjectDictType,NULL);

    // 初始化一个table放一些c的函数，后面这个table被全局命名为redis
    lua_newtable(lua);

    // 这里是相当于设置   redis.call = luaRedisCallCommand
    lua_pushstring(lua,"call");
    lua_pushcfunction(lua,luaRedisCallCommand);
    lua_settable(lua,-3);

    // redis.pcall = luaRedisPCallCommand
    lua_pushstring(lua,"pcall");
    lua_pushcfunction(lua,luaRedisPCallCommand);
    lua_settable(lua,-3);

    // redis.log
    lua_pushstring(lua,"log");
    lua_pushcfunction(lua,luaLogCommand);
    lua_settable(lua,-3);


    /* redis.sha1hex */
    lua_pushstring(lua, "sha1hex");
    lua_pushcfunction(lua, luaRedisSha1hexCommand);
    lua_settable(lua, -3);

    /* redis.error_reply and redis.status_reply */
    lua_pushstring(lua, "error_reply");
    lua_pushcfunction(lua, luaRedisErrorReplyCommand);
    lua_settable(lua, -3);
    lua_pushstring(lua, "status_reply");
    lua_pushcfunction(lua, luaRedisStatusReplyCommand);
    lua_settable(lua, -3);

    /* redis.replicate_commands */
    lua_pushstring(lua, "replicate_commands");
    lua_pushcfunction(lua, luaRedisReplicateCommandsCommand);
    lua_settable(lua, -3);

    /* redis.set_repl and associated flags. */
    lua_pushstring(lua,"set_repl");
    lua_pushcfunction(lua,luaRedisSetReplCommand);
    lua_settable(lua,-3);

    lua_pushstring(lua,"REPL_NONE");
    lua_pushnumber(lua,PROPAGATE_NONE);
    lua_settable(lua,-3);

    lua_pushstring(lua,"REPL_AOF");
    lua_pushnumber(lua,PROPAGATE_AOF);
    lua_settable(lua,-3);

    lua_pushstring(lua,"REPL_SLAVE");
    lua_pushnumber(lua,PROPAGATE_REPL);
    lua_settable(lua,-3);

    lua_pushstring(lua,"REPL_ALL");
    lua_pushnumber(lua,PROPAGATE_AOF|PROPAGATE_REPL);
    lua_settable(lua,-3);

    /* redis.debug */
    lua_pushstring(lua,"debug");
    lua_pushcfunction(lua,luaRedisDebugCommand);
    lua_settable(lua,-3);

    // 将上面的这个table命名为redis, 放入全局命名空间
    lua_setglobal(lua,"redis");

    // 设置math.random = redis_math_random, math.randomseed = redis_math_randomseed
    lua_getglobal(lua,"math");
    lua_pushstring(lua,"random");
    lua_pushcfunction(lua,redis_math_random);
    lua_settable(lua,-3);

    lua_pushstring(lua,"randomseed");
    lua_pushcfunction(lua,redis_math_randomseed);
    lua_settable(lua,-3);

    lua_setglobal(lua,"math");

    // 添加了一个lua写的脚本函数，用来比较大小(在之前的array.sort里有使用)
    {
        char *compare_func =    "function __redis__compare_helper(a,b)\n"
                                "  if a == false then a = '' end\n"
                                "  if b == false then b = '' end\n"
                                "  return a<b\n"
                                "end\n";
        luaL_loadbuffer(lua,compare_func,strlen(compare_func),"@cmp_func_def");
        lua_pcall(lua,0,0,0);
    }

    // 添加一个error handler脚本
    {
        char *errh_func =       "local dbg = debug\n"
                                "function __redis__err__handler(err)\n"
                                "  local i = dbg.getinfo(2,'nSl')\n"
                                "  if i and i.what == 'C' then\n"
                                "    i = dbg.getinfo(3,'nSl')\n"
                                "  end\n"
                                "  if i then\n"
                                "    return i.source .. ':' .. i.currentline .. ': ' .. err\n"
                                "  else\n"
                                "    return err\n"
                                "  end\n"
                                "end\n";
        luaL_loadbuffer(lua,errh_func,strlen(errh_func),"@err_handler_def");
        lua_pcall(lua,0,0,0);
    }

    // 创建一个fake client用来执行lua命令
    if (server.lua_client == NULL) {
        server.lua_client = createClient(-1);
        server.lua_client->flags |= CLIENT_LUA;
    }
    server.lua = lua;


#### void luaSetGlobalArray(lua_State *lua, char *var, robj **elev, int elec)
将字符串数组elev变成lua的数组，以var为名字放入全局空间

    lua_newtable(lua);
    for (j = 0; j < elec; j++) {
        lua_pushlstring(lua,(char*)elev[j]->ptr,sdslen(elev[j]->ptr));
        lua_rawseti(lua,-2,j+1);
    }
    lua_setglobal(lua,var);


#### int redis_math_random (lua_State *L)
random的实现用来替换math.random([m[, n]]), 可以是0个,1个,2个参数

  // 先获取[0, 1)区间的随机数
  lua_Number r = (lua_Number)(redisLrand48()%REDIS_LRAND48_MAX) /
                                (lua_Number)REDIS_LRAND48_MAX;
  // 根据参数个数0,1,2分别处理
  switch (lua_gettop(L)) {
    // 0个直接返回r
    case 0:
      lua_pushnumber(L, r);
      break;
    }
    // 返回1到u之间的数, u必须比1大
    case 1: {
      int u = luaL_checkint(L, 1);
      luaL_argcheck(L, 1<=u, 1, "interval is empty");
      lua_pushnumber(L, floor(r*u)+1);
      break;
    }
    // 返回[l, u], 满足l <= u
    case 2: {
      int l = luaL_checkint(L, 1);
      int u = luaL_checkint(L, 2);
      luaL_argcheck(L, l<=u, 2, "interval is empty");
      lua_pushnumber(L, floor(r*(u-l+1))+l);
      break;
    }
    default: return luaL_error(L, "wrong number of arguments");
  }
  return 1;


#### int luaCreateFunction(client *c, lua_State *lua, char *funcname, robj *body)
将body的内容包装成一个lua function并载入.

    // body字符串拼接成function
    funcdef = sdscat(funcdef,"function ");
    funcdef = sdscatlen(funcdef,funcname,42);
    funcdef = sdscatlen(funcdef,"() ",3);
    funcdef = sdscatlen(funcdef,body->ptr,sdslen(body->ptr));
    funcdef = sdscatlen(funcdef,"\nend",4);

    if (luaL_loadbuffer(lua,funcdef,sdslen(funcdef),"@user_script")) {
        // DEAL_WITH_ERROR
        return C_ERR;
    }
    sdsfree(funcdef);
    if (lua_pcall(lua,0,0,0)) {
        // DEAL_WITH_ERROR
        return C_ERR;
    }
    // 添加到server.lua_scripts里面去
    {
        int retval = dictAdd(server.lua_scripts,
                             sdsnewlen(funcname+2,40),body);
        serverAssertWithInfo(c,NULL,retval == DICT_OK);
        incrRefCount(body);
    }


#### void luaMaskCountHook(lua_State *lua, lua_Debug *ar)
lua可以通过lua_sethook这个api来设置一些hook, count hook在执行函数的时候会被调用.
这里利用这个hook来检测script执行是不是超时了并作处理.

    elapsed = mstime() - server.lua_time_start;
    if (elapsed >= server.lua_time_limit && server.lua_timedout == 0) {
        serverLog(LL_WARNING,"Lua slow script detected: still in execution after %lld milliseconds. You can try killing the script using the SCRIPT KILL command.",elapsed);
        server.lua_timedout = 1;
        // 这里没有特别明白这么做的作用, 应该是避免又调度到这里继续执行很慢的脚本
         aeDeleteFileEvent(server.el, server.lua_caller->fd, AE_READABLE);
    }
    // 让eventloop处理一些请求，这样才有机会让另外一个client来kill script
    if (server.lua_timedout) processEventsWhileBlocked();
    if (server.lua_kill) {
        serverLog(LL_WARNING,"Lua script killed by user with SCRIPT KILL.");
        lua_pushstring(lua,"Script killed by user with SCRIPT KILL...");
        lua_error(lua);
    }

#### void evalGenericCommand(client *c, int evalsha)
eval和evalsha命令的处理.

    lua_State *lua = server.lua;
    char funcname[43];
    long long numkeys;
    int delhook = 0, err;

    // 设置random的seed, 这样master和client的random序列就都一样了
    redisSrand48(0);
    // 标记脚本里是否有write操作，有write之后就不能做random相关的操作了
    server.lua_random_dirty = 0;
    server.lua_write_dirty = 0;
    server.lua_replicate_commands = server.lua_always_replicate_commands;
    server.lua_multi_emitted = 0;
    server.lua_repl = PROPAGATE_AOF|PROPAGATE_REPL;

    // 获取参数里key的个数 (删减了参数合法性检查的部分)
    if (getLongLongFromObjectOrReply(c,c->argv[2],&numkeys,NULL) != C_OK)
        return;

    // eval和evalsha在这里有不同的处理, eval需要调用sha1hex计算
    funcname[0] = 'f';
    funcname[1] = '_';
    if (!evalsha) {
        sha1hex(funcname+2,c->argv[1]->ptr,sdslen(c->argv[1]->ptr));
    } else {
        int j;
        char *sha = c->argv[1]->ptr;
        // sha里面的大小全部转换成小写
        for (j = 0; j < 40; j++)
            funcname[j+2] = (sha[j] >= 'A' && sha[j] <= 'Z') ?
                sha[j]+('a'-'A') : sha[j];
        funcname[42] = '\0';
    }

    // 获取error handler并入栈
    lua_getglobal(lua, "__redis__err__handler");

    // 尝试获取函数，如果是eval并且未获取到则需要创建函数
    lua_getglobal(lua, funcname);
    if (lua_isnil(lua,-1)) {
        lua_pop(lua,1);         // 将nil出栈
        // 如果是evalsha并且函数不存在，就报错返回客户端
        if (evalsha) {
            lua_pop(lua,1);
            addReply(c, shared.noscripterr);
            return;
        }
        // 如果是eval, 就创建函数
        if (luaCreateFunction(c,lua,funcname,c->argv[1]) == C_ERR) {
            lua_pop(lua,1);
            return;
        }
        // 创建成功后，再次获取函数并入栈
        lua_getglobal(lua, funcname);
        serverAssert(!lua_isnil(lua,-1));
    }
    // 将参数里的KEYS, ARGV设置成lua全局变量(这样前面的函数里可以用)
    luaSetGlobalArray(lua,"KEYS",c->argv+3,numkeys);
    luaSetGlobalArray(lua,"ARGV",c->argv+3+numkeys,c->argc-3-numkeys);
    selectDb(server.lua_client,c->db->id);

    // 初始化开始时间及其它状态，设置一个hook，在脚本执行过程中会经常调用这个hook, 在这个hook里可以做很多检查，比如超时处理之类的
    server.lua_caller = c;
    server.lua_time_start = mstime();
    server.lua_kill = 0;
    if (server.lua_time_limit > 0 && server.masterhost == NULL &&
        ldb.active == 0)
    {
        lua_sethook(lua,luaMaskCountHook,LUA_MASKCOUNT,100000);
        delhook = 1;
    } else if (ldb.active) {
        lua_sethook(server.lua,luaLdbLineHook,LUA_MASKLINE|LUA_MASKCOUNT,100000);
        delhook = 1;
    }
    // 执行函数并设置error handler
    err = lua_pcall(lua,0,1,-2);

    // 执行完了的一些清理工作
    if (delhook) lua_sethook(lua,NULL,0,0);
    if (server.lua_timedout) {
        server.lua_timedout = 0;
        // 在hook的timeout处理里临时将当前client给从eventloop拿掉了, 这里给重新添加回去
        aeCreateFileEvent(server.el,c->fd,AE_READABLE,
                          readQueryFromClient,c);
    }
    server.lua_caller = NULL;

    // 调用luagc
    #define LUA_GC_CYCLE_PERIOD 50
    {
        static long gc_count = 0;

        gc_count++;
        if (gc_count == LUA_GC_CYCLE_PERIOD) {
            lua_gc(lua,LUA_GCSTEP,LUA_GC_CYCLE_PERIOD);
            gc_count = 0;
        }
    }

    if (err) {
        addReplyErrorFormat(c,"Error running script (call to %s): %s\n",
            funcname, lua_tostring(lua,-1));
        lua_pop(lua,2);     // 将reply和error handler给pop掉
    } else {
        luaReplyToRedisReply(c,lua);
        lua_pop(lua,1);     // 将error handler给pop掉
    }

    if (server.lua_replicate_commands) {
        preventCommandPropagation(c);
        if (server.lua_multi_emitted) {
            robj *propargv[1];
            propargv[0] = createStringObject("EXEC",4);
            alsoPropagate(server.execCommand,c->db->id,propargv,1,
                PROPAGATE_AOF|PROPAGATE_REPL);
            decrRefCount(propargv[0]);
        }
    }
    if (evalsha && !server.lua_replicate_commands) {
        if (!replicationScriptCacheExists(c->argv[1]->ptr)) {
            robj *script = dictFetchValue(server.lua_scripts,c->argv[1]->ptr);

            replicationScriptCacheAdd(c->argv[1]->ptr);
            serverAssertWithInfo(c,NULL,script != NULL);
            rewriteClientCommandArgument(c,0,
                resetRefCount(createStringObject("EVAL",4)));
            rewriteClientCommandArgument(c,1,script);
            forceCommandPropagation(c,PROPAGATE_REPL|PROPAGATE_AOF);
        }
    }

#### void scriptCommand(client *c)
这个函数是对script flush/exists/load/kill/debug的处理.
flush就是释放server.lua_scripts的所有现存script然后将server.lua关闭再重新初始化.
exists检查某个sha值的script是否存在.
load是载入一个script但是不执行.
kill这里是试图杀死当前运行的script，如果已经有写操作了就不能杀掉了，没有的话，这里是标记一下.
等lua运行的时候自己的hook里去根据这个标记与否来退出(lua_error)

后面debug的部分先都忽略了.


### Reference:
1. [github: redis scripting](https://github.com/antirez/redis/blob/unstable/src/scripting.c)
2. [devdocs: lua api](http://devdocs.io/lua~5.3/index)
