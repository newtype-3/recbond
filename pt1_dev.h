/* -*- tab-width: 4; indent-tabs-mode: nil -*- */
#ifndef _PT1_DEV_H_
#define _PT1_DEV_H_


// BonDriverテーブル(pathは各自で変更すること)
// 衛星波
char * bsdev[] = {
	(char *)"/usr/local/lib/BonDriver/BonDriver_LinuxPT-S0.so",
	(char *)"/usr/local/lib/BonDriver/BonDriver_LinuxPT-S1.so",
	(char *)"/usr/local/lib/BonDriver/BonDriver_LinuxPT-S2.so",
	(char *)"/usr/local/lib/BonDriver/BonDriver_LinuxPT-S3.so"
};
// 衛星波Proxy
char * bsdev_proxy[] = {
	(char *)"/usr/local/lib/BonDriver/BonDriver_Proxy-S0.so",
	(char *)"/usr/local/lib/BonDriver/BonDriver_Proxy-S1.so",
	(char *)"/usr/local/lib/BonDriver/BonDriver_Proxy-S2.so",
	(char *)"/usr/local/lib/BonDriver/BonDriver_Proxy-S3.so"
};

// 地上波
char * isdb_t_dev[] = {
	(char *)"/usr/local/lib/BonDriver/BonDriver_LinuxPT-T0.so",
	(char *)"/usr/local/lib/BonDriver/BonDriver_LinuxPT-T1.so",
	(char *)"/usr/local/lib/BonDriver/BonDriver_LinuxPT-T2.so",
	(char *)"/usr/local/lib/BonDriver/BonDriver_LinuxPT-T3.so"
};
// 地上波Proxy
char * isdb_t_dev_proxy[] = {
	(char *)"/usr/local/lib/BonDriver/BonDriver_Proxy-T0.so",
	(char *)"/usr/local/lib/BonDriver/BonDriver_Proxy-T1.so",
	(char *)"/usr/local/lib/BonDriver/BonDriver_Proxy-T2.so",
	(char *)"/usr/local/lib/BonDriver/BonDriver_Proxy-T3.so"
};

#define NUM_BSDEV				(sizeof(bsdev)/sizeof(char *))
#define NUM_BSDEV_PROXY			(sizeof(bsdev_proxy)/sizeof(char *))
#define NUM_ISDB_T_DEV			(sizeof(isdb_t_dev)/sizeof(char *))
#define NUM_ISDB_T_DEV_PROXY	(sizeof(isdb_t_dev_proxy)/sizeof(char *))

#endif
