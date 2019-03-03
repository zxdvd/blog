+++
title = "代码阅读: redis-server-config"
date = "2018-04-29"
categories = ["Linux"]
tags = ["read-code", "c", "redis"]
slug = "read-code-redis-server-config"
Authors = "Xudong"
+++

集中解读下redisServer这个struct，也就是到处可见的server.XXX.
定义在server.h里，初始化在server.c的initServerConfig函数里.

    struct redisServer {
        pid_t pid;                  // 主进程id
        char *configfile;           // 配置文件绝对路径
        char *executable;           // 可执行文件绝对路径(重启需要)
        char **exec_argv;           // 启动时候的参数(重启需要)
        // hz是定义cronjob的周期，也就是一秒执行多少次serverCron(默认是10)
        int hz;
        redisDb *db;
        dict *commands;             // command table(可以被修改，比如rename)
        dict *orig_commands;        // 初始化时候的command table(不可修改)
        aeEventLoop *el;            // eventloop
        unsigned lruclock:LRU_BITS; // LRU时钟
        int shutdown_asap;          // 立即退出程序
        int activerehashing;        /* Incremental rehash in serverCron() */
        char *requirepass;          // 连接时候需要的密码, 可为空(不需要密码)
        char *pidfile;
        int arch_bits;              // 32位系统还是64位, (sizeof(long) == 8) ? 64: 32
        int cronloops;              // cron计数器(执行了多少次)
        char runid[CONFIG_RUN_ID_SIZE+1];  /* ID always different at every exec. */
        int sentinel_mode;          // 是否是sentinel模式
        /* Modules */
        dict *moduleapi;            /* Exported APIs dictionary for modules. */
        list *loadmodule_queue;     /* List of modules to load at startup. */
        // 网络部分
        int port;
        int tcp_backlog;            // tcp的backlog配置
        char *bindaddr[CONFIG_BINDADDR_MAX]; /* Addresses we should bind to */
        int bindaddr_count;         /* Number of addresses in server.bindaddr[] */
        char *unixsocket;
        mode_t unixsocketperm;      // unixsocket的权限
        int ipfd[CONFIG_BINDADDR_MAX]; /* TCP socket file descriptors */
        int ipfd_count;             /* Used slots in ipfd[] */
        int sofd;                   /* Unix socket file descriptor */
        int cfd[CONFIG_BINDADDR_MAX];/* Cluster bus listening socket */
        int cfd_count;              /* Used slots in cfd[] */
        list *clients;              // 所有的clients(双向链表)
        list *clients_to_close;     // 待关闭的clients(关闭是异步处理的)
        list *clients_pending_write; /* There is to write or install handler. */
        list *slaves, *monitors;    /* List of slaves and MONITORs */
        client *current_client; /* Current client, only used on crash report */
        int clients_paused;         /* True if clients are currently paused */
        mstime_t clients_pause_end_time; /* Time when we undo clients_paused */
        char neterr[ANET_ERR_LEN];   /* Error buffer for anet.c */
        dict *migrate_cached_sockets;/* MIGRATE cached sockets */
        uint64_t next_client_id;    /* Next client unique ID. Incremental. */
        int protected_mode;         // 保护模式(不接受外部的连接)
        /* RDB / AOF loading information */
        int loading;                // 是否正在从磁盘loading数据
        off_t loading_total_bytes;
        off_t loading_loaded_bytes;
        time_t loading_start_time;
        off_t loading_process_events_interval_bytes;
        /* Fast pointers to often looked up command */
        struct redisCommand *delCommand, *multiCommand, *lpushCommand, *lpopCommand,
                            *rpopCommand, *sremCommand, *execCommand;
        // stat相关
        time_t stat_starttime;          // 服务启动时间
        long long stat_numcommands;     // 处理的指令数量
        long long stat_numconnections;  /* Number of connections received */
        long long stat_expiredkeys;     // 过期的key的总数量
        long long stat_evictedkeys;     // evicted的key的总数量
        long long stat_keyspace_hits;   // 成功找到的key的数量
        long long stat_keyspace_misses; // key miss数量
        size_t stat_peak_memory;        // 记录的内存使用峰值
        long long stat_fork_time;       /* Time needed to perform latest fork() */
        double stat_fork_rate;          /* Fork rate in GB/sec. */
        long long stat_rejected_conn;   /* Clients rejected because of maxclients */
        long long stat_sync_full;       /* Number of full resyncs with slaves. */
        long long stat_sync_partial_ok; /* Number of accepted PSYNC requests. */
        long long stat_sync_partial_err;/* Number of unaccepted PSYNC requests. */
        list *slowlog;                  /* SLOWLOG list of commands */
        long long slowlog_entry_id;     /* SLOWLOG current entry ID */
        long long slowlog_log_slower_than; /* SLOWLOG time limit (to get logged) */
        unsigned long slowlog_max_len;     /* SLOWLOG max number of items logged */
        size_t resident_set_size;       /* RSS sampled in serverCron(). */
        long long stat_net_input_bytes; /* Bytes read from network. */
        long long stat_net_output_bytes; /* Bytes written to network. */
        /* The following two are used to track instantaneous metrics, like
         * number of operations per second, network traffic. */
        struct {
            long long last_sample_time; /* Timestamp of last sample in ms */
            long long last_sample_count;/* Count in last sample */
            long long samples[STATS_METRIC_SAMPLES];
            int idx;
        } inst_metric[STATS_METRIC_COUNT];
        // 配置
        int verbosity;                  // 日志级别
        int maxidletime;                // 客户端超时时间(对应timeout配置)
        int tcpkeepalive;               // tcp的keepalive时间，默认300s
        int active_expire_enabled;      /* Can be disabled for testing purposes. */
        size_t client_max_querybuf_len; /* Limit for client query buffer length */
        int dbnum;                      // db总数量, 默认16个
        int supervised;                 // 是否是被systemd之类的supervisor启动
        int supervised_mode;            /* See SUPERVISED_* */
        int daemonize;
        clientBufferLimitsConfig client_obuf_limits[CLIENT_TYPE_OBUF_COUNT];
        /* AOF persistence */
        int aof_state;                  /* AOF_(ON|OFF|WAIT_REWRITE) */
        int aof_fsync;                  /* Kind of fsync() policy */
        char *aof_filename;             /* Name of the AOF file */
        int aof_no_fsync_on_rewrite;    /* Don't fsync if a rewrite is in prog. */
        int aof_rewrite_perc;           /* Rewrite AOF if % growth is > M and... */
        off_t aof_rewrite_min_size;     /* the AOF file is at least N bytes. */
        off_t aof_rewrite_base_size;    /* AOF size on latest startup or rewrite. */
        off_t aof_current_size;         /* AOF current size. */
        int aof_rewrite_scheduled;      /* Rewrite once BGSAVE terminates. */
        pid_t aof_child_pid;            /* PID if rewriting process */
        list *aof_rewrite_buf_blocks;   /* Hold changes during an AOF rewrite. */
        sds aof_buf;      /* AOF buffer, written before entering the event loop */
        int aof_fd;       /* File descriptor of currently selected AOF file */
        int aof_selected_db; /* Currently selected DB in AOF */
        time_t aof_flush_postponed_start; /* UNIX time of postponed AOF flush */
        time_t aof_last_fsync;            /* UNIX time of last fsync() */
        time_t aof_rewrite_time_last;   /* Time used by last AOF rewrite run. */
        time_t aof_rewrite_time_start;  /* Current AOF rewrite start time. */
        int aof_lastbgrewrite_status;   /* C_OK or C_ERR */
        unsigned long aof_delayed_fsync;  /* delayed AOF fsync() counter */
        int aof_rewrite_incremental_fsync;/* fsync incrementally while rewriting? */
        int aof_last_write_status;      /* C_OK or C_ERR */
        int aof_last_write_errno;       /* Valid if aof_last_write_status is ERR */
        int aof_load_truncated;         /* Don't stop on unexpected AOF EOF. */
        /* AOF pipes used to communicate between parent and child during rewrite. */
        int aof_pipe_write_data_to_child;
        int aof_pipe_read_data_from_parent;
        int aof_pipe_write_ack_to_parent;
        int aof_pipe_read_ack_from_child;
        int aof_pipe_write_ack_to_child;
        int aof_pipe_read_ack_from_parent;
        int aof_stop_sending_diff;     /* If true stop sending accumulated diffs
                                          to child process. */
        sds aof_child_diff;             /* AOF diff accumulator child side. */
        /* RDB persistence */
        long long dirty;                /* Changes to DB from the last save */
        long long dirty_before_bgsave;  /* Used to restore dirty on failed BGSAVE */
        pid_t rdb_child_pid;            /* PID of RDB saving child */
        struct saveparam *saveparams;   /* Save points array for RDB */
        int saveparamslen;              /* Number of saving points */
        char *rdb_filename;             /* Name of RDB file */
        int rdb_compression;            /* Use compression in RDB? */
        int rdb_checksum;               /* Use RDB checksum? */
        time_t lastsave;                /* Unix time of last successful save */
        time_t lastbgsave_try;          /* Unix time of last attempted bgsave */
        time_t rdb_save_time_last;      /* Time used by last RDB save run. */
        time_t rdb_save_time_start;     /* Current RDB save start time. */
        int rdb_bgsave_scheduled;       /* BGSAVE when possible if true. */
        int rdb_child_type;             /* Type of save by active child. */
        int lastbgsave_status;          /* C_OK or C_ERR */
        int stop_writes_on_bgsave_err;  /* Don't allow writes if can't BGSAVE */
        int rdb_pipe_write_result_to_parent; /* RDB pipes used to return the state */
        int rdb_pipe_read_result_from_child; /* of each slave in diskless SYNC. */
        /* Propagation of commands in AOF / replication */
        redisOpArray also_propagate;    /* Additional command to propagate. */
        /* Logging */
        char *logfile;                  /* Path of log file */
        int syslog_enabled;             /* Is syslog enabled? */
        char *syslog_ident;             /* Syslog ident */
        int syslog_facility;            /* Syslog facility */
        /* Replication (master) */
        int slaveseldb;                 /* Last SELECTed DB in replication output */
        long long master_repl_offset;   /* Global replication offset */
        int repl_ping_slave_period;     /* Master pings the slave every N seconds */
        char *repl_backlog;             /* Replication backlog for partial syncs */
        long long repl_backlog_size;    /* Backlog circular buffer size */
        long long repl_backlog_histlen; /* Backlog actual data length */
        long long repl_backlog_idx;     /* Backlog circular buffer current offset */
        long long repl_backlog_off;     /* Replication offset of first byte in the
                                           backlog buffer. */
        time_t repl_backlog_time_limit; /* Time without slaves after the backlog
                                           gets released. */
        time_t repl_no_slaves_since;    /* We have no slaves since that time.
                                           Only valid if server.slaves len is 0. */
        int repl_min_slaves_to_write;   /* Min number of slaves to write. */
        int repl_min_slaves_max_lag;    /* Max lag of <count> slaves to write. */
        int repl_good_slaves_count;     /* Number of slaves with lag <= max_lag. */
        int repl_diskless_sync;         /* Send RDB to slaves sockets directly. */
        int repl_diskless_sync_delay;   /* Delay to start a diskless repl BGSAVE. */
        /* Replication (slave) */
        char *masterauth;               /* AUTH with this password with master */
        char *masterhost;               /* Hostname of master */
        int masterport;                 /* Port of master */
        int repl_timeout;               /* Timeout after N seconds of master idle */
        client *master;     /* Client that is master for this slave */
        client *cached_master; /* Cached master to be reused for PSYNC. */
        int repl_syncio_timeout; /* Timeout for synchronous I/O calls */
        int repl_state;          /* Replication status if the instance is a slave */
        off_t repl_transfer_size; /* Size of RDB to read from master during sync. */
        off_t repl_transfer_read; /* Amount of RDB read from master during sync. */
        off_t repl_transfer_last_fsync_off; /* Offset when we fsync-ed last time. */
        int repl_transfer_s;     /* Slave -> Master SYNC socket */
        int repl_transfer_fd;    /* Slave -> Master SYNC temp file descriptor */
        char *repl_transfer_tmpfile; /* Slave-> master SYNC temp file name */
        time_t repl_transfer_lastio; /* Unix time of the latest read, for timeout */
        int repl_serve_stale_data; /* Serve stale data when link is down? */
        int repl_slave_ro;          /* Slave is read only? */
        time_t repl_down_since; /* Unix time at which link with master went down */
        int repl_disable_tcp_nodelay;   /* Disable TCP_NODELAY after SYNC? */
        int slave_priority;             /* Reported in INFO and used by Sentinel. */
        int slave_announce_port;        /* Give the master this listening port. */
        char *slave_announce_ip;        /* Give the master this ip address. */
        char repl_master_runid[CONFIG_RUN_ID_SIZE+1];  /* Master run id for PSYNC.*/
        long long repl_master_initial_offset;         /* Master PSYNC offset. */
        int repl_slave_lazy_flush;          /* Lazy FLUSHALL before loading DB? */
        /* Replication script cache. */
        dict *repl_scriptcache_dict;        /* SHA1 all slaves are aware of. */
        list *repl_scriptcache_fifo;        /* First in, first out LRU eviction. */
        unsigned int repl_scriptcache_size; /* Max number of elements. */
        /* Synchronous replication. */
        list *clients_waiting_acks;         /* Clients waiting in WAIT command. */
        int get_ack_from_slaves;            /* If true we send REPLCONF GETACK. */
        /* Limits */
        unsigned int maxclients;            /* Max number of simultaneous clients */
        unsigned long long maxmemory;   /* Max number of memory bytes to use */
        int maxmemory_policy;           /* Policy for key eviction */
        int maxmemory_samples;          /* Pricision of random sampling */
        unsigned int lfu_log_factor;    /* LFU logarithmic counter factor. */
        unsigned int lfu_decay_time;    /* LFU counter decay factor. */
        /* Blocked clients */
        unsigned int bpop_blocked_clients; /* Number of clients blocked by lists */
        list *unblocked_clients; /* list of clients to unblock before next loop */
        list *ready_keys;        /* List of readyList structures for BLPOP & co */
        /* Sort parameters - qsort_r() is only available under BSD so we
         * have to take this state global, in order to pass it to sortCompare() */
        int sort_desc;
        int sort_alpha;
        int sort_bypattern;
        int sort_store;
        /* Zip structure config, see redis.conf for more information  */
        size_t hash_max_ziplist_entries;
        size_t hash_max_ziplist_value;
        size_t set_max_intset_entries;
        size_t zset_max_ziplist_entries;
        size_t zset_max_ziplist_value;
        size_t hll_sparse_max_bytes;
        /* List parameters */
        int list_max_ziplist_size;
        int list_compress_depth;
        /* time cache */
        time_t unixtime;    /* Unix time sampled every cron cycle. */
        long long mstime;   /* Like 'unixtime' but with milliseconds resolution. */
        /* Pubsub */
        dict *pubsub_channels;  /* Map channels to list of subscribed clients */
        list *pubsub_patterns;  /* A list of pubsub_patterns */
        int notify_keyspace_events; /* Events to propagate via Pub/Sub. This is an
                                       xor of NOTIFY_... flags. */
        /* Cluster */
        int cluster_enabled;      /* Is cluster enabled? */
        mstime_t cluster_node_timeout; /* Cluster node timeout. */
        char *cluster_configfile; /* Cluster auto-generated config file name. */
        struct clusterState *cluster;  /* State of the cluster */
        int cluster_migration_barrier; /* Cluster replicas migration barrier. */
        int cluster_slave_validity_factor; /* Slave max data age for failover. */
        int cluster_require_full_coverage; /* If true, put the cluster down if
                                              there is at least an uncovered slot.*/
        char *cluster_announce_ip;  /* IP address to announce on cluster bus. */
        int cluster_announce_port;     /* base port to announce on cluster bus. */
        int cluster_announce_bus_port; /* bus port to announce on cluster bus. */
        /* Scripting */
        lua_State *lua; /* The Lua interpreter. We use just one for all clients */
        client *lua_client;   /* The "fake client" to query Redis from Lua */
        client *lua_caller;   /* The client running EVAL right now, or NULL */
        dict *lua_scripts;         /* A dictionary of SHA1 -> Lua scripts */
        mstime_t lua_time_limit;  /* Script timeout in milliseconds */
        mstime_t lua_time_start;  /* Start time of script, milliseconds time */
        int lua_write_dirty;  /* True if a write command was called during the
                                 execution of the current script. */
        int lua_random_dirty; /* True if a random command was called during the
                                 execution of the current script. */
        int lua_replicate_commands; /* True if we are doing single commands repl. */
        int lua_multi_emitted;/* True if we already proagated MULTI. */
        int lua_repl;         /* Script replication flags for redis.set_repl(). */
        int lua_timedout;     /* True if we reached the time limit for script
                                 execution. */
        int lua_kill;         /* Kill the script if true. */
        int lua_always_replicate_commands; /* Default replication type. */
        /* Lazy free */
        int lazyfree_lazy_eviction;
        int lazyfree_lazy_expire;
        int lazyfree_lazy_server_del;
        /* Latency monitor */
        long long latency_monitor_threshold;
        dict *latency_events;
        /* Assert & bug reporting */
        const char *assert_failed;
        const char *assert_file;
        int assert_line;
        int bug_report_start; /* True if bug report header was already logged. */
        int watchdog_period;  /* Software watchdog period in ms. 0 = off */
        /* System hardware info */
        size_t system_memory_size;  /* Total memory in system as reported by OS */
    };


### Reference:
1. [github: redis server config](https://github.com/antirez/redis/blob/unstable/src/server.h)
