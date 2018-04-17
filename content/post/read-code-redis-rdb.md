+++
title = "代码阅读: redis-rdb"
date = "2018-03-24"
categories = ["Linux"]
tags = ["read-code", "c", "redis"]
slug = "read-code-redis-rdb"
Authors = "Xudong"
+++

rdb处理的模块，同样是写入文件，rdb跟aof文件的保存格式完全不一样.

#### static int rdbWriteRaw(rio *rdb, void *p, size_t len)
如果rdb不为NULL, 就调用rioWrite将p指向的位置写入len个字节到rdb.

#### int rdbSaveLen(rio *rdb, uint64_t len)
将len这个表示长度的数值写入到rdb, 这里为了节省空间, 根据len的大小范围做了不少特别处理.

在rdb.h里面有解释如下

    00|XXXXXX => 高2位为00表示len长度不超过2^6-1, 占1字节
    01|XXXXXX XXXXXXXX =>  高2位为01, 后面14位表示len长度，占2字节
    10|000000 [32 bit integer] => 0x80表示32位的len, 共占用5字节
    10|000001 [64 bit integer] => 0x81表示64字节的len, 共占用5字节
    11|OBKIND => 特别编码(比如RDB_ENC_INT8, INT16, INT32, LZF)

这个函数就是把len转变成上面的字节然后写入rdb, 对于多个字节的int都转换成网络字节序(htonl)写入的.

#### int rdbLoadLenByRef(rio *rdb, int *isencoded, uint64_t *lenptr)
这个是rdbSaveLen的逆操作, 先读取一个字节，根据头2bit来判断长度是6位还是14位还是32或者64位，也可能是特别编码(11开头), 那么isencoded就设置为1.

这里还需要注意的是多字节的时候要把网络字节序转换成host字节序(ntohl).

#### int rdbEncodeInteger(long long value, unsigned char *enc)
将long long类型根据大小范围特别处理下，处理成上面len里的那种特别编码的形式，就是第一个字节头两位是11, 后6为表示是INT8, INT16, INT32, 这样如果value比较小的话能节省不少空间.
这里是用小端的字节序保存的.

#### void *rdbLoadIntegerObject(rio *rdb, int enctype, int flags, size_t *lenptr)
从rdb获取一个int值, 根据enctype确定是INT8, INT16还是INT32, 然后读取并组装数据.
根据flags(plain, sds, encode三种)来确定返回形式.

#### int rdbTryIntegerEncoding(char *s, size_t len, unsigned char *enc)
这里试图将s转换成long long, 然后调用rdbEncodeInteger.

#### size_t rdbSaveLzfBlob(rio *rdb, void *data, size_t compress_len, size_t original_len)
将LZF串写入rdb, 先写入一个表示类型的字节(RDB_ENCVAL<<6|RDB_ENC_LZF), 然后依次写入LZF串的长度，压缩前的原始长度，在写入整个串的内容, 最后返回总的写入的字节数.

#### ssize_t rdbSaveLzfStringObject(rio *rdb, unsigned char *s, size_t len)
尝试压缩一个长度为len的字符串s, 压缩成功后调用rdbSaveLzfBlob写入LZF串并返回写入字节数.

#### void *rdbLoadLzfStringObject(rio *rdb, int flags, size_t *lenptr)
rdbSaveLzfBlob的逆过程, 读取长度以及对应长度的字节数然后解压返回.

#### ssize_t rdbSaveRawString(rio *rdb, unsigned char *s, size_t len)
写入长度为len的字符串, 如果长度小于等于11，先尝试是不是数字，如果是的话转换成数字并写入;
然后如果server配置成支持压缩并且len大于20就用LZF压缩并写入; 如果这张失败了就采用最原始的方式，先写入长度，再写入整个串.

#### ssize_t rdbSaveLongLongAsStringObject(rio *rdb, long long value)
写入long long类型，先尝试rdbEncodeInteger, 如果超过了INT32就转换成string的形式并写入.

#### void *rdbGenericLoadStringObject(rio *rdb, int flags, size_t *lenptr)
读取字符串, 先读取长度和类型然后分别处理.

    len = rdbLoadLen(rdb,&isencoded);
    if (isencoded) {
        switch(len) {
        case RDB_ENC_INT8:
        case RDB_ENC_INT16:
        case RDB_ENC_INT32:
            return rdbLoadIntegerObject(rdb,len,flags,lenptr);
        case RDB_ENC_LZF:
            return rdbLoadLzfStringObject(rdb,flags,lenptr);
        default:
            rdbExitReportCorruptRDB("Unknown RDB string encoding type %d",len);
        }
    }
    if (plain || sds) {
        void *buf = plain ? zmalloc(len) : sdsnewlen(NULL,len);
        if (lenptr) *lenptr = len;
        if (len && rioRead(rdb,buf,len) == 0) {
            if (plain)
                zfree(buf);
            else
                sdsfree(buf);
            return NULL;
        }
        return buf;
    } else {
        robj *o = encode ? createStringObject(NULL,len) :
                           createRawStringObject(NULL,len);
        if (len && rioRead(rdb,o->ptr,len) == 0) {
            decrRefCount(o);
            return NULL;
        }
        return o;
    }

#### int rdbSaveDoubleValue(rio *rdb, double val)
将double转换成string写入, 另外253表示不是数字, 254和255分别表示正无穷和负无穷.

#### int rdbSaveObjectType(rio *rdb, robj *o)
写入robj的类型, STRING, LIST(QUICKLIST), SET(INTSET, HT), ZSET(ZIPLIST, SKIPLIST), HASH(ZIPLIST, HT), MODULE.

#### ssize_t rdbSaveObject(rio *rdb, robj *o)
写入一个robj, 根据6种类型分别处理.

    ssize_t n = 0, nwritten = 0;
    if (o->type == OBJ_STRING) {
        if ((n = rdbSaveStringObject(rdb,o)) == -1) return -1;
        nwritten += n;
    } else if (o->type == OBJ_LIST) {
        if (o->encoding == OBJ_ENCODING_QUICKLIST) {
            quicklist *ql = o->ptr;
            quicklistNode *node = ql->head;
            // 先写入总长度
            if ((n = rdbSaveLen(rdb,ql->len)) == -1) return -1;
            nwritten += n;
            // 一个个node节点分别处理, 根据压缩和非压缩情况
            do {
                if (quicklistNodeIsCompressed(node)) {
                    void *data;
                    size_t compress_len = quicklistGetLzf(node, &data);
                    if ((n = rdbSaveLzfBlob(rdb,data,compress_len,node->sz)) == -1) return -1;
                    nwritten += n;
                } else {
                    if ((n = rdbSaveRawString(rdb,node->zl,node->sz)) == -1) return -1;
                    nwritten += n;
                }
            } while ((node = node->next));
        }
    } else if (o->type == OBJ_SET) {
        // SET的处理
        if (o->encoding == OBJ_ENCODING_HT) {
            dict *set = o->ptr;
            dictIterator *di = dictGetIterator(set);
            dictEntry *de;

            if ((n = rdbSaveLen(rdb,dictSize(set))) == -1) return -1;
            nwritten += n;

            while((de = dictNext(di)) != NULL) {
                sds ele = dictGetKey(de);
                if ((n = rdbSaveRawString(rdb,(unsigned char*)ele,sdslen(ele)))
                    == -1) return -1;
                nwritten += n;
            }
            dictReleaseIterator(di);
        } else if (o->encoding == OBJ_ENCODING_INTSET) {
            size_t l = intsetBlobLen((intset*)o->ptr);

            if ((n = rdbSaveRawString(rdb,o->ptr,l)) == -1) return -1;
            nwritten += n;
        }
    } else if (o->type == OBJ_ZSET) {
        /* Save a sorted set value */
        if (o->encoding == OBJ_ENCODING_ZIPLIST) {
            size_t l = ziplistBlobLen((unsigned char*)o->ptr);

            if ((n = rdbSaveRawString(rdb,o->ptr,l)) == -1) return -1;
            nwritten += n;
        } else if (o->encoding == OBJ_ENCODING_SKIPLIST) {
            zset *zs = o->ptr;
            dictIterator *di = dictGetIterator(zs->dict);
            dictEntry *de;

            if ((n = rdbSaveLen(rdb,dictSize(zs->dict))) == -1) return -1;
            nwritten += n;

            while((de = dictNext(di)) != NULL) {
                sds ele = dictGetKey(de);
                double *score = dictGetVal(de);

                if ((n = rdbSaveRawString(rdb,(unsigned char*)ele,sdslen(ele)))
                    == -1) return -1;
                nwritten += n;
                if ((n = rdbSaveBinaryDoubleValue(rdb,*score)) == -1) return -1;
                nwritten += n;
            }
            dictReleaseIterator(di);
        }
    } else if (o->type == OBJ_HASH) {
        /* Save a hash value */
        if (o->encoding == OBJ_ENCODING_ZIPLIST) {
            size_t l = ziplistBlobLen((unsigned char*)o->ptr);

            if ((n = rdbSaveRawString(rdb,o->ptr,l)) == -1) return -1;
            nwritten += n;

        } else if (o->encoding == OBJ_ENCODING_HT) {
            dictIterator *di = dictGetIterator(o->ptr);
            dictEntry *de;

            if ((n = rdbSaveLen(rdb,dictSize((dict*)o->ptr))) == -1) return -1;
            nwritten += n;

            while((de = dictNext(di)) != NULL) {
                sds field = dictGetKey(de);
                sds value = dictGetVal(de);

                if ((n = rdbSaveRawString(rdb,(unsigned char*)field,
                        sdslen(field))) == -1) return -1;
                nwritten += n;
                if ((n = rdbSaveRawString(rdb,(unsigned char*)value,
                        sdslen(value))) == -1) return -1;
                nwritten += n;
            }
            dictReleaseIterator(di);
        }
    }
    return nwritten;
}

#### int rdbSaveKeyValuePair(rio *rdb, robj *key, robj *val, long long expiretime, long long now)
写入一对key, value的robj, 以及对应的expiretime(如果有的话).

#### int rdbSaveAuxField(rio *rdb, void *key, size_t keylen, void *val, size_t vallen)
写入一对key, value字符串.

#### int rdbSaveRio(rio *rdb, int *error)
这个是将整个redis数据写入到rdb的过程处理.

    // 先写入一个magic string, 里面包含版本号.
    snprintf(magic,sizeof(magic),"REDIS%04d",RDB_VERSION);
    if (rdbWriteRaw(rdb,magic,9) == -1) goto werr;
    // 然后写入一些辅助字段，比如redis版本，32位平台还是64位, 当前时间，内存占用
    if (rdbSaveInfoAuxFields(rdb) == -1) goto werr;
    // 遍历所有db一个个的处理
    for (j = 0; j < server.dbnum; j++) {
        redisDb *db = server.db+j;
        dict *d = db->dict;
        if (dictSize(d) == 0) continue;
        di = dictGetSafeIterator(d);
        if (!di) return C_ERR;
        // 将当前db的id用一个OPCODE_SELECTDB来标记.
        if (rdbSaveType(rdb,RDB_OPCODE_SELECTDB) == -1) goto werr;
        if (rdbSaveLen(rdb,j) == -1) goto werr;
        // 接着记录dict, expires的size大小，也是用一个OPCODE_RESIZEDB在前面标记
        // 这里的目的是在于恢复的时候，提前知道了dict的大小，可以先初始化到这个大小，而不是从很小的开始然后慢慢resize起来
        // 这里有设置一个最大的限制，但是并没有限制dict里面存的key, value对的最大个数，总之这里就是一个hint
        uint32_t db_size, expires_size;
        db_size = (dictSize(db->dict) <= UINT32_MAX) ?
                                dictSize(db->dict) :
                                UINT32_MAX;
        expires_size = (dictSize(db->expires) <= UINT32_MAX) ?
                                dictSize(db->expires) :
                                UINT32_MAX;
        if (rdbSaveType(rdb,RDB_OPCODE_RESIZEDB) == -1) goto werr;
        if (rdbSaveLen(rdb,db_size) == -1) goto werr;
        if (rdbSaveLen(rdb,expires_size) == -1) goto werr;

        // 循环处理每一个key, value对
        while((de = dictNext(di)) != NULL) {
            sds keystr = dictGetKey(de);
            robj key, *o = dictGetVal(de);
            long long expire;

            initStaticStringObject(key,keystr);
            expire = getExpire(db,&key);
            // 将key, value, expire一起写入
            if (rdbSaveKeyValuePair(rdb,&key,o,expire,now) == -1) goto werr;
        }
        dictReleaseIterator(di);
    }
    di = NULL;
    // 写入标示结束的OPCODE_EOF
    if (rdbSaveType(rdb,RDB_OPCODE_EOF) == -1) goto werr;
    // 写入校验和
    cksum = rdb->cksum;
    memrev64ifbe(&cksum);
    if (rioWrite(rdb,&cksum,8) == 0) goto werr;

#### int rdbSave(char *filename)
调用rdbSaveRio将redis数据写入到一个临时文件，然后rename到filename.

#### int rdbSaveBackground(char *filename)
rdbSave是在当前线程里同步save, 会导致所有的clients都block，显然线上环境是不能这么弄的.
而这个就是一个异步的后台save的过程，通过fork进程实现整个内存的copy, 然后save的就是fork这一时间点的时候的数据的snapshot.
这样不会阻塞主线程.
可以注意到fork之后主线程调用了一个`updateDictResizePolicy()`,目的是希望禁止dict的resize，因为resize过程会导致大量的页的改动，fork进程的copy on write效果会大打折扣, 造成内存使用倍增.

#### robj *rdbLoadObject(int rdbtype, rio *rdb)
rdbSaveObject的逆过程，从rdb里读出一个robj.

#### int rdbSaveToSlavesSockets(void)
这个是用rdb来同步的，向slave sockets里同步数据. 核心部分代码大致如下

    // 遍历所有slave找出需要同步的那些slaves
    listRewind(server.slaves,&li);
    while((ln = listNext(&li))) {
        client *slave = ln->value;
        if (slave->replstate == SLAVE_STATE_WAIT_BGSAVE_START) {
            fds[numfds++] = slave->fd;
            replicationSetupSlaveForFullResync(slave,getPsyncInitialOffset());
            anetBlock(NULL,slave->fd);
            anetSendTimeout(NULL,slave->fd,server.repl_timeout*1000);
        }
    }
    if ((childpid = fork()) == 0) {
        // 将多个fdsets初始化成rio(fdsets是rio支持的一种)
        rioInitWithFdset(&slave_sockets,fds,numfds);
        retval = rdbSaveRioWithEOFMark(&slave_sockets,NULL);
    }

### Reference:
1. [github: redis rdb](https://github.com/antirez/redis/blob/unstable/src/rdb.c)
