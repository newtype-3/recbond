/* -*- tab-width: 4; indent-tabs-mode: nil -*- */
#ifndef _RECPT1_H_
#define _RECPT1_H_

#include "typedef.h"

#define CHTYPE_SATELLITE    0        /* satellite digital */
#define CHTYPE_GROUND       1        /* terrestrial digital */
#define CHTYPE_BonNUMBER    2        // BonDriver number
#define MAX_DRIVER          256
#define MAX_QUEUE           8192
#define MAX_READ_SIZE       (188 * 256) // BonDriver_DVB.hのTS_BUFSIZEと同値
#define WRITE_SIZE          (1024 * 1024 * 2)
#define TRUE                1
#define FALSE               0

typedef struct _BUFSZ {
    int size;
    u_char buffer[MAX_READ_SIZE];	// 凡ドラ側でB25デコード時はこのサイズより大きくなるので注意
} BUFSZ;

typedef struct _QUEUE_T {
    unsigned int in;        // 次に入れるインデックス
    unsigned int out;        // 次に出すインデックス
    unsigned int size;        // キューのサイズ
    unsigned int num_avail;    // 満タンになると 0 になる
    unsigned int num_used;    // 空っぽになると 0 になる
    pthread_mutex_t mutex;
    pthread_cond_t cond_avail;    // データが満タンのときに待つための cond
    pthread_cond_t cond_used;    // データが空のときに待つための cond
    BUFSZ *buffer[1];    // バッファポインタ
} QUEUE_T;

typedef struct _BON_CHANNEL_TABLE {
    char *driver;    // チャンネルファイル
    DWORD dwSpace;   // スペース指定(デフォルトは0)
    DWORD dwChannel; // チャンネル指定(デフォルトは-1)
    char *parm_channel; // パラメータで受ける値
} BON_CHANNEL_TABLE;

#endif
