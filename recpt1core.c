#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <dlfcn.h>
#include "typedef.h"
#include "IBonDriver2.h"
#include "AribChannel.h"
#include "recpt1core.h"
#include "version.h"

/* globals */
boolean f_exit = FALSE;
char *alldev[MAX_DRIVER];   // Proxyでないドライバ(衛星波+地上波)
int num_alldev;
char *alldev_proxy[MAX_DRIVER];   // Proxyなドライバ(衛星波+地上波)
int num_alldev_proxy;
char *bsdev[MAX_DRIVER];			// 衛星波
int num_bsdev;
char *bsdev_proxy[MAX_DRIVER];		// 衛星波Proxy
int num_bsdev_proxy;
char *isdb_t_dev[MAX_DRIVER];		// 地上波
int num_isdb_t_dev;
char *isdb_t_dev_proxy[MAX_DRIVER];	// 地上波Proxy
int num_isdb_t_dev_proxy;
BON_CHANNEL_TABLE bon_channel_table = {NULL, 0, 0, NULL};

int
set_driver_list(void)
{
	FILE *fp;
	char *p, buf[256];
	ssize_t len;

	if ((len = readlink("/proc/self/exe", buf, sizeof(buf) - 8)) == -1)
		return 2;
	buf[len] = '\0';
	strcat(buf, ".conf");

	fp = fopen(buf, "r");
	if (fp == NULL) {
		fprintf(stderr, "Cannot open '%s'\n", buf);
		return 1;
	}

	int i = 0;
	int ia = 0;
	int iap = 0;
	int ib = 0;
	int ibp = 0;
	int it = 0;
	int itp = 0;
	while (fgets(buf, sizeof(buf), fp) && i < MAX_DRIVER - 1) {
		if (buf[0] == ';')
			continue;
		p = buf + strlen(buf) - 1;
		while ((p >= buf) && (*p == '\r' || *p == '\n'))
			*p-- = '\0';
		if (p < buf)
			continue;

		int n = 0;
		char *cp[3];
		p = cp[n++] = buf;
		while ((p = strchr(p, '\t'))) {
			*p++ = '\0';
			cp[n++] = p;
			if (n > 2) {
				break;
			}
		}
		if (n > 1) {
			switch(cp[0][0]) {
			case 'S':
				if (cp[0][1] == 'P') {
					bsdev_proxy[ibp] = strdup(cp[1]);
					alldev_proxy[iap] = bsdev_proxy[ibp];
					iap++;
					ibp++;
				} else {
					bsdev[ib] = strdup(cp[1]);
					alldev[ia] = bsdev[ib];
					ia++;
					ib++;
				}
				break;
			case 'T':
				if (cp[0][1] == 'P') {
					isdb_t_dev_proxy[itp] = strdup(cp[1]);
					alldev_proxy[iap] = isdb_t_dev_proxy[itp];
					iap++;
					itp++;
				} else {
					isdb_t_dev[it] = strdup(cp[1]);
					alldev[ia] = isdb_t_dev[it];
					ia++;
					it++;
				}
				break;
			}
			i++;
		}
	}

	fclose(fp);
	num_alldev = ia;
	num_alldev_proxy = iap;
	num_bsdev = ib;
	num_bsdev_proxy = ibp;
	num_isdb_t_dev = it;
	num_isdb_t_dev_proxy = itp;

	return 0;
}

/* lookup frequency conversion table*/
BON_CHANNEL_TABLE *
searchrecoff(char *channel, char *driver)
{
	FILE *fp;
	char *p, bufd[256], bufl[256];

	DWORD dwSpace = 0;
	DWORD dwChannel = 0;
	boolean find = FALSE;

	if (channel[0] == 'B' && channel[1] == 'o' && channel[2] == 'n') {
		char *ch_tmp = channel + 3;
		while (isdigit((int)*ch_tmp))
		{
			dwChannel *= 10;
			dwChannel += (DWORD)(*ch_tmp++ - '0');
		}
		if (*ch_tmp == '_')
		{
			dwSpace = dwChannel;
			dwChannel = 0;
			ch_tmp++;
			while (isdigit((int)*ch_tmp))
			{
				dwChannel *= 10;
				dwChannel += (DWORD)(*ch_tmp++ - '0');
			}
			if (*ch_tmp == '\0')
			{
				find = TRUE;
			}
		} else if (*ch_tmp == '\0') {
			find = TRUE;
		}
		strcpy(bufd, "[direct]");
	} else {
		unsigned long tsid = 0;
		if (strncasecmp(channel, "tsid", 4) == 0) {
			char* strtsid = channel + 4;
			char* endp;
			tsid = strtoul(strtsid, &endp, 0);
			if (*endp != '\0') {
				tsid = 0;
			}
		}

		strncpy(bufd, driver, sizeof(bufd) - 8);
		bufd[sizeof(bufd) - 8] = '\0';
		strcat(bufd, ".ch");

		fp = fopen(bufd, "r");
		if (fp == NULL) {
			fprintf(stderr, "Cannot open '%s'\n", bufd);
			return NULL;
		}

		while (fgets(bufl, sizeof(bufl), fp)) {
			if (bufl[0] == ';')
				continue;
			p = bufl + strlen(bufl) - 1;
			while ((p >= bufl) && (*p == '\r' || *p == '\n'))
				*p-- = '\0';
			if (p < bufl)
				continue;

			int n = 0;
			char *cp[6];
			p = cp[n++] = bufl;
			while ((p = strchr(p, '\t'))) {
				*p++ = '\0';
				cp[n++] = p;
				if (n > 6) {
					break;
				}
			}
			if (n > 2) {
				if (tsid == 0) {
					if (strcmp(channel, cp[0]) == 0) {
						dwSpace = (DWORD)strtol(cp[1], NULL, 10);
						dwChannel = (DWORD)strtol(cp[2], NULL, 10);
						find = TRUE;
						break;
					}
				} else if (n > 4) {
					if (strtoul(cp[4], NULL, 0) == tsid) {
						dwSpace = (DWORD)strtol(cp[1], NULL, 10);
						dwChannel = (DWORD)strtol(cp[2], NULL, 10);
						find = TRUE;
						break;
					}
				}
			}
		}

		fclose(fp);
	}


	if(find){
		fprintf(stderr, "find '%s': channel=%s, dwSpace=%d, dwChannel=%d \n", bufd, channel, dwSpace, dwChannel);
		bon_channel_table.driver = driver;
		bon_channel_table.dwSpace = dwSpace;
		bon_channel_table.dwChannel = dwChannel;
		bon_channel_table.parm_channel = channel;
		return &bon_channel_table;
	}else {
		fprintf(stderr, "Cannot find '%s': channel=%s\n", bufd, channel);
		return NULL;
	}
}

int
open_tuner(thread_data *tdata, char *driver)
{
	// モジュールロード
	tdata->hModule = dlopen(driver, RTLD_LAZY);
	if(!tdata->hModule) {
		fprintf(stderr, "Cannot open tuner driver: %s\ndlopen error: %s\n", driver, dlerror());
		return -1;
	}

	// インスタンス作成
	tdata->pIBon = tdata->pIBon2 = NULL;
	char *err;
	IBonDriver *(*f)() = (IBonDriver *(*)())dlsym(tdata->hModule, "CreateBonDriver");
	if ((err = dlerror()) == NULL)
	{
		tdata->pIBon = f();
		if (tdata->pIBon)
			tdata->pIBon2 = dynamic_cast<IBonDriver2 *>(tdata->pIBon);
	}
	else
	{
		fprintf(stderr, "dlsym error: %s\n", err);
		dlclose(tdata->hModule);
		return -2;
	}
	if (!tdata->pIBon || !tdata->pIBon2)
	{
		fprintf(stderr, "CreateBonDriver error: tdata->pIBon[%p] tdata->pIBon2[%p]\n", tdata->pIBon, tdata->pIBon2);
		dlclose(tdata->hModule);
		return -3;
	}

	// ここから実質のチューナオープン & TS取得処理
	BOOL b = tdata->pIBon->OpenTuner();
	if (!b)
	{
		tdata->pIBon->Release();
		dlclose(tdata->hModule);
		return -4;
	}else
		tdata->tfd = TRUE;

#if 0	// linuxのBonDriverはOpenTuner()内で自動変更・SetLnbPower()未実装
	/* power on LNB */
	if(tdata->lnb != -1 && tdata->table->type == CHTYPE_SATELLITE) {
		if(!tdata->pIBon->SetLnbPower(tdata->lnb>0 ? TRUE : FALSE)) {
			fprintf(stderr, "Power on LNB failed: %s\n", driver);
		}
	}
#endif
return 0;
}

int
close_tuner(thread_data *tdata)
{
	int rv = 0;

	if(tdata->table){
		tdata->table = NULL;
	}
	if(tdata->hModule == NULL)
		return rv;

#if 0	// linuxのBonDriverはCloseTuner()内で自動変更・SetLnbPower()未実装
	if(tdata->lnb > 0 && tdata->table->type == CHTYPE_SATELLITE) {
		if(!tdata->pIBon->SetLnbPower(FALSE)) {
			rv = 1;
		}
	}
#endif
	// チューナクローズ
	if(tdata->tfd){
		tdata->pIBon->CloseTuner();
		tdata->tfd = FALSE;
	}
	// インスタンス解放 & モジュールリリース
	tdata->pIBon->Release();
	dlclose(tdata->hModule);
	tdata->hModule = NULL;

	return rv;
}


void
calc_cn(thread_data *tdata, boolean use_bell)
{
	double	CNR = (double)tdata->pIBon->GetSignalLevel();

	if(use_bell) {
		int bell = 0;

		if(CNR >= 30.0)
			bell = 3;
		else if(CNR >= 15.0 && CNR < 30.0)
			bell = 2;
		else if(CNR < 15.0)
			bell = 1;
		fprintf(stderr, "\rC/N = %fdB ", CNR);
		do_bell(bell);
	}
	else {
		fprintf(stderr, "\rC/N = %fdB", CNR);
	}
}

void
show_channels(void)
{
	FILE *f;
	char *home;
	char buf[255], filename[255];

	fprintf(stderr, "Available Channels:\n");

	home = getenv("HOME");
	sprintf(filename, "%s/.recpt1-channels", home);
	f = fopen(filename, "r");
	if(f) {
		while(fgets(buf, 255, f))
			fprintf(stderr, "%s", buf);
		fclose(f);
	}
	else
		fprintf(stderr, "13-62: Terrestrial Channels\n");

	fprintf(stderr, "101ch: NHK BS1\n");
	fprintf(stderr, "103ch: NHK BS Premium\n");
	fprintf(stderr, "141ch: BS Nittele\n");
	fprintf(stderr, "151ch: BS Asahi\n");
	fprintf(stderr, "161ch: BS-TBS\n");
	fprintf(stderr, "171ch: BS Japan\n");
	fprintf(stderr, "181ch: BS Fuji\n");
	fprintf(stderr, "191ch: WOWOW Prime\n");
	fprintf(stderr, "192ch: WOWOW Live\n");
	fprintf(stderr, "193ch: WOWOW Cinema\n");
	fprintf(stderr, "200ch: Star Channel1\n");
	fprintf(stderr, "201ch: Star Channel2\n");
	fprintf(stderr, "202ch: Star Channel3\n");
	fprintf(stderr, "211ch: BS11 Digital\n");
	fprintf(stderr, "222ch: TwellV\n");
	fprintf(stderr, "231ch: Housou Daigaku 1\n");
	fprintf(stderr, "232ch: Housou Daigaku 2\n");
	fprintf(stderr, "233ch: Housou Daigaku 3\n");
	fprintf(stderr, "234ch: Green Channel\n");
	fprintf(stderr, "236ch: BS Animax\n");
	fprintf(stderr, "238ch: FOX bs238\n");
	fprintf(stderr, "241ch: BS SkyPer!\n");
	fprintf(stderr, "242ch: J SPORTS 1\n");
	fprintf(stderr, "243ch: J SPORTS 2\n");
	fprintf(stderr, "244ch: J SPORTS 3\n");
	fprintf(stderr, "245ch: J SPORTS 4\n");
	fprintf(stderr, "251ch: BS Tsuri Vision\n");
	fprintf(stderr, "252ch: IMAGICA BS\n");
	fprintf(stderr, "255ch: Nihon Eiga Senmon Channel\n");
	fprintf(stderr, "256ch: Disney Channel\n");
	fprintf(stderr, "258ch: Dlife\n");
	fprintf(stderr, "C13-C63: CATV Channels\n");
	fprintf(stderr, "CS2-CS24: CS Channels\n");
	fprintf(stderr, "B0-: BonDriver Channel\n");
	fprintf(stderr, "BS1_0-BS25_1: BS Channels(Transport)\n");
	fprintf(stderr, "0x4000-0x7FFF: BS/CS Channels(TSID)\n");
}


int
parse_time(const char * rectimestr, int *recsec)
{
	/* indefinite */
	if(!strcmp("-", rectimestr)) {
		*recsec = -1;
		return 0;
	}
	/* colon */
	else if(strchr(rectimestr, ':')) {
		int n1, n2, n3;
		if(sscanf(rectimestr, "%d:%d:%d", &n1, &n2, &n3) == 3)
			*recsec = n1 * 3600 + n2 * 60 + n3;
		else if(sscanf(rectimestr, "%d:%d", &n1, &n2) == 2)
			*recsec = n1 * 3600 + n2 * 60;
		else
			return 1; /* unsuccessful */

		return 0;
	}
	/* HMS */
	else {
		char *tmpstr;
		char *p1, *p2;
		int  flag;

		if( *rectimestr == '-' ){
			rectimestr++;
			flag = 1;
		}else
			flag = 0;
		tmpstr = strdup(rectimestr);
		p1 = tmpstr;
		while(*p1 && !isdigit(*p1))
			p1++;

		/* hour */
		if((p2 = strchr(p1, 'H')) || (p2 = strchr(p1, 'h'))) {
			*p2 = '\0';
			*recsec += (int)strtol(p1, NULL, 10) * 3600;
			p1 = p2 + 1;
			while(*p1 && !isdigit(*p1))
				p1++;
		}

		/* minute */
		if((p2 = strchr(p1, 'M')) || (p2 = strchr(p1, 'm'))) {
			*p2 = '\0';
			*recsec += (int)strtol(p1, NULL, 10) * 60;
			p1 = p2 + 1;
			while(*p1 && !isdigit(*p1))
				p1++;
		}

		/* second */
		*recsec += (int)strtol(p1, NULL, 10);
		if( flag )
			*recsec *= -1;

		free(tmpstr);

		return 0;
	} /* else */

	return 1; /* unsuccessful */
}

void
do_bell(int bell)
{
	int i;
	for(i=0; i < bell; i++) {
		fprintf(stderr, "\a");
		usleep(400000);
	}
}

int
tune(char *channel, thread_data *tdata, char *driver)
{
	/* get channel */
	BON_CHANNEL_TABLE *table_tmp;
	char *dri_tmp = driver;
	char tmpc;

	/* get driver from channel */
	if(sscanf(channel, "P%*1[ST]%*d%1[_]", &tmpc) == 1
	|| sscanf(channel, "%*1[ST]%*d%1[_]", &tmpc) == 1
	|| sscanf(channel, "P%*1[ST]%1[_]", &tmpc) == 1
	|| sscanf(channel, "%*1[ST]%1[_]", &tmpc) == 1
	){
		for(dri_tmp = channel; *channel != '_'; channel++){
		}
		*channel++ = '\0';
	}

	/* open tuner */
	int aera;
	char **tuner;
	int num_devs = 0;
	if(dri_tmp && *dri_tmp == 'P'){
		// proxy
		aera = 1;
		dri_tmp++;
	}else
		aera = 0;
	if(dri_tmp && *dri_tmp != '\0'){
		if(*dri_tmp == 'S'){
			if(aera == 0){
				tuner = bsdev;
				num_devs = num_bsdev;
			}else{
				tuner = bsdev_proxy;
				num_devs = num_bsdev_proxy;
			}
			dri_tmp++;
		}else if(*dri_tmp == 'T'){
			if(aera == 0){
				tuner = isdb_t_dev;
				num_devs = num_isdb_t_dev;
			}else{
				tuner = isdb_t_dev_proxy;
				num_devs = num_isdb_t_dev_proxy;
			}
			dri_tmp++;
		}
	}else{
		if(aera == 0){
			tuner = alldev;
			num_devs = num_alldev;
		}else{
			tuner = alldev_proxy;
			num_devs = num_alldev_proxy;
		}
	}

	if(dri_tmp && *dri_tmp != '\0') {
		/* case 1: specified tuner driver */
		int num = 0;
		int code;

		if(isdigit(*dri_tmp)){
			do{
				num = num * 10 + *dri_tmp++ - '0';
			}while(isdigit(*dri_tmp));
			if(*dri_tmp == '\0' && num+1 <= num_devs)
				driver = tuner[num];
		}

		if((code = open_tuner(tdata, driver))){
			if(code == -4)
				fprintf(stderr, "OpenTuner erro: %s\n", driver);
			return 1;
		}
		/* tune to specified channel */
		if((table_tmp = searchrecoff(channel, driver)) == NULL){
			close_tuner(tdata);
			fprintf(stderr, "Invalid Channel: %s\n", channel);
			return 1;
		}
#if 0
		DWORD m_dwChannel = tdata->pIBon2->GetCurChannel();
		if(m_dwChannel != ARIB_CH_ERROR && m_dwChannel != dwSendBonNum){
			fprintf(stderr, "Tuner has been used: %s\n", driver);
			close_tuner(tdata);
			return 1;
		}
#endif
		while(tdata->pIBon2->SetChannel(table_tmp->dwSpace, table_tmp->dwChannel) == FALSE) {
			if(tdata->tune_persistent) {
				if(f_exit) {
					close_tuner(tdata);
					return 1;
				}
				fprintf(stderr, "No signal. Still trying: %s\n", driver);
			}
			else {
				close_tuner(tdata);
				fprintf(stderr, "Cannot tune to the specified channel: %s\n", driver);
				return 1;
			}
		}
		fprintf(stderr, "driver = %s\n", driver);
	}
	else {
		/* case 2: loop around available devices */
		boolean tuned = FALSE;
		int lp;
		
		for(lp = 0; lp < num_devs; lp++) {
			int count = 0;

			if(open_tuner(tdata, tuner[lp]) == 0) {
				if((table_tmp = searchrecoff(channel, tuner[lp])) == NULL){
					close_tuner(tdata);
					continue;
				}
				// 使用中チェック・違うチャンネルを選局している場合はスキップ
				DWORD m_dwChannel = tdata->pIBon2->GetCurChannel();
				if(m_dwChannel != ARIB_CH_ERROR){
					if(m_dwChannel != table_tmp->dwChannel){
						close_tuner(tdata);
						continue;
					}
				}else{
					/* tune to specified channel */
					if(tdata->tune_persistent) {
						while(tdata->pIBon2->SetChannel(table_tmp->dwSpace, table_tmp->dwChannel) == FALSE && count < MAX_RETRY) {
							if(f_exit) {
								close_tuner(tdata);
								return 1;
							}
							fprintf(stderr, "No signal. Still trying: %s\n", tuner[lp]);
							count++;
						}

						if(count >= MAX_RETRY) {
							close_tuner(tdata);
							count = 0;
							continue;
						}
					} /* tune_persistent */
					else {
						if(tdata->pIBon2->SetChannel(table_tmp->dwSpace, table_tmp->dwChannel) == FALSE) {
							close_tuner(tdata);
							continue;
						}
					}
				}

				tuned = TRUE;
				fprintf(stderr, "driver = %s\n", tuner[lp]);
				break; /* found suitable tuner */
			}
		}

		/* all tuners cannot be used */
		if (tuned == FALSE) {
			fprintf(stderr, "Cannot tune to the specified channel\n");
			return 1;
		}
	}
	tdata->table = table_tmp;
	// TS受信開始待ち
	timespec ts;
	ts.tv_sec = 0;
	ts.tv_nsec = 50 * 1000 * 1000;	// 50ms
	{
	int lop = 0;
	do{
		nanosleep(&ts, NULL);
	}while(tdata->pIBon->GetReadyCount() < 2 && ++lop < 20);
	}

	if(!tdata->tune_persistent) {
		/* show signal strength */
		calc_cn(tdata, FALSE);
	}

	return 0; /* success */
}
