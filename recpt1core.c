#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <dlfcn.h>
#include "typedef.h"
#include "IBonDriver2.h"
#include "AribChannel.h"
#include "recpt1core.h"
#include "version.h"
#include "pt1_dev.h"

/* globals */
boolean f_exit = FALSE;
BON_CHANNEL_SET channel_set;

/* lookup frequency conversion table*/
BON_CHANNEL_SET *
searchrecoff(char *channel)
{
	DWORD node;
	DWORD slot;
	DWORD dwBonChannel = channelAribToBon(channel);

	if( dwBonChannel == ARIB_CH_ERROR )
		return NULL;
	memset(channel_set.parm_freq, 0, 16);
	channel_set.bon_num = -1;
	channel_set.set_freq = (int)dwBonChannel;
	switch(dwBonChannel>>16){
		case BON_CHANNEL:
			node = (int)dwBonChannel;
			channel_set.bon_num = node;
			channel_set.type = CHTYPE_BonNUMBER;
			sprintf(channel_set.parm_freq, "B%d", node);
			break;
		case ARIB_GROUND:
		case ARIB_CATV:
			channel_set.bon_num = (int)(dwBonChannel & 0xFFFFU);
			channel_set.type = CHTYPE_GROUND;
			sprintf(channel_set.parm_freq, "%s", channel);
			break;
		case ARIB_BS:
			node = (dwBonChannel & 0x01f0U) >> 4;
			slot = dwBonChannel & 0x0007U;
			channel_set.type = CHTYPE_SATELLITE;
			sprintf(channel_set.parm_freq, "BS%d_%d", node, slot);
			break;
		case ARIB_BS_SID:
			channel_set.type = CHTYPE_SATELLITE;
			break;
		case ARIB_CS:
			node = dwBonChannel & 0x00ffU;
			channel_set.type = CHTYPE_SATELLITE;
			sprintf(channel_set.parm_freq, "CS%d", node);
			break;
		case ARIB_TSID:
			node = (dwBonChannel & 0x01f0U) >> 4;
			channel_set.type = CHTYPE_SATELLITE;
			if( (dwBonChannel & 0xf008U) == 0x4000U ){
				slot = dwBonChannel & 0x0007U;
				if( node == 15 )
					slot--;
				sprintf(channel_set.parm_freq, "BS%d_%d", node, slot);
			}else{
				sprintf(channel_set.parm_freq, "CS%d", node);
			}
			break;
	}
	return &channel_set;
}

int
get_bon_channel(char *channel, char *driver, DWORD *dwSpace, DWORD *dwChannel)
{
	FILE *fp;
	char *p, bufd[256], bufl[256];
	ssize_t len;

	DWORD tmp_dwSpace;
	DWORD tmp_dwChannel;

	strncpy(bufd, driver, sizeof(bufd) - 8);
	bufd[sizeof(bufd) - 8] = '\0';
	strcat(bufd, ".ch");

	fp = fopen(bufd, "r");
	if (fp == NULL) {
		fprintf(stderr, "Cannot open '%s'\n", bufd);
		return 1;
	}

	boolean find = FALSE;
	int i = 0;
	len = sizeof(bufl) - 1;
	while (fgets(bufl, sizeof(bufl), fp)) {
		if (bufl[0] == ';')
			continue;
		p = bufl + strlen(bufl) - 1;
		while ((p >= bufl) && (*p == '\r' || *p == '\n'))
			*p-- = '\0';
		if (p < bufl)
			continue;

		int n = 0;
		char *cp[3];
		boolean bOk = FALSE;
		p = cp[n++] = bufl;
		while (1) {
			p = strchr(p, '\t');
			if (p) {
				*p++ = '\0';
				cp[n++] = p;
				if (n > 2) {
					bOk = TRUE;
					break;
				}
			} else
				break;
		}
		if (bOk) {
			if (strcmp(channel, cp[0]) == 0) {
				tmp_dwSpace = (DWORD)strtol(cp[1], NULL, 10);
				tmp_dwChannel = (DWORD)strtol(cp[2], NULL, 10);
				find = TRUE;
				break;
			}
		}
	}

	fclose(fp);

	if(find){
		fprintf(stderr, "find '%s': channel=%s, dwSpace=%d, dwChannel=%d \n", bufd, channel, tmp_dwSpace, tmp_dwChannel);
		*dwSpace = tmp_dwSpace;
		*dwChannel = tmp_dwChannel;
		return 0;
	}else {
		fprintf(stderr, "Cannot find '%s': channel=%s\n", bufd, channel);
		return 2;
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
	BON_CHANNEL_SET *table_tmp = searchrecoff(channel);
	if(table_tmp == NULL) {
		fprintf(stderr, "Invalid Channel: %s\n", channel);
		return 1;
	}

	/* open tuner */
	char *dri_tmp = driver;
	int aera;
	char **tuner;
	char *tmp_driver;
	DWORD tmp_dwSpace;
	DWORD tmp_dwChannel;
	int num_devs = 0;
	if(dri_tmp && *dri_tmp == 'P'){
		// proxy
		aera = 1;
		dri_tmp++;
	}else
		aera = 0;
	if(dri_tmp){
		if(*dri_tmp == 'S'){
			if(aera == 0){
				tuner = bsdev;
				num_devs = NUM_BSDEV;
			}else{
				tuner = bsdev_proxy;
				num_devs = NUM_BSDEV_PROXY;
			}
			dri_tmp++;
		}else if(*dri_tmp == 'T'){
			if(aera == 0){
				tuner = isdb_t_dev;
				num_devs = NUM_ISDB_T_DEV;
			}else{
				tuner = isdb_t_dev_proxy;
				num_devs = NUM_ISDB_T_DEV_PROXY;
			}
			dri_tmp++;
		}
	}

	if(dri_tmp && *dri_tmp != '\0') {
		/* case 1: specified tuner driver */
		int num = 0;
		int code;

		if(!isdigit(*dri_tmp)){
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
		if(get_bon_channel(channel, driver, &tmp_dwSpace, &tmp_dwChannel)){
			close_tuner(tdata);
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
		while(tdata->pIBon2->SetChannel(tmp_dwSpace, tmp_dwChannel) == FALSE) {
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
		tmp_driver = driver;
	}
	else {
		/* case 2: loop around available devices */
		boolean tuned = FALSE;
		int lp;

		if(num_devs == 0){
			switch(table_tmp->type){
				case CHTYPE_BonNUMBER:
				default:
					fprintf(stderr, "No driver name\n");
					close_tuner(tdata);
					return 1;
				case CHTYPE_SATELLITE:
					if(aera == 0){
						tuner = bsdev;
						num_devs = NUM_BSDEV;
					}else{
						tuner = bsdev_proxy;
						num_devs = NUM_BSDEV_PROXY;
					}
					break;
				case CHTYPE_GROUND:
					if(aera == 0){
						tuner = isdb_t_dev;
						num_devs = NUM_ISDB_T_DEV;
					}else{
						tuner = isdb_t_dev_proxy;
						num_devs = NUM_ISDB_T_DEV_PROXY;
					}
					break;
			}
		}

		for(lp = 0; lp < num_devs; lp++) {
			int count = 0;

			if(open_tuner(tdata, tuner[lp]) == 0) {
				if(get_bon_channel(channel, tuner[lp], &tmp_dwSpace, &tmp_dwChannel)){
					close_tuner(tdata);
					continue;
				}
				// 使用中チェック・違うチャンネルを選局している場合はスキップ
				DWORD m_dwChannel = tdata->pIBon2->GetCurChannel();
				if(m_dwChannel != ARIB_CH_ERROR){
					if(m_dwChannel != tmp_dwChannel){
						close_tuner(tdata);
						continue;
					}
				}else{
					/* tune to specified channel */
					if(tdata->tune_persistent) {
						while(tdata->pIBon2->SetChannel(tmp_dwSpace, tmp_dwChannel) == FALSE && count < MAX_RETRY) {
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
						if(tdata->pIBon2->SetChannel(tmp_dwSpace, tmp_dwChannel) == FALSE) {
							close_tuner(tdata);
							continue;
						}
					}
				}

				tuned = TRUE;
				fprintf(stderr, "driver = %s\n", tuner[lp]);
				tmp_driver = tuner[lp];
				break; /* found suitable tuner */
			}
		}

		/* all tuners cannot be used */
		if (tuned == FALSE) {
			fprintf(stderr, "Cannot tune to the specified channel\n");
			return 1;
		}
	}
	tdata->driver = tmp_driver;
	tdata->dwSpace = tmp_dwSpace;
	tdata->dwChannel = tmp_dwChannel;
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
