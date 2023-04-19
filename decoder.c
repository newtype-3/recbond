#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "decoder.h"

#ifdef HAVE_LIBARIB25

DECODER *b25_startup(decoder_options *opt)
{
	DECODER *dec = (DECODER *)calloc(1, sizeof(DECODER));
	int code;
	const char *err = NULL;

	dec->b25 = create_arib_std_b25();
	if (!dec->b25) {
		err = "create_arib_std_b25 failed";
		goto error;
	}

	code = dec->b25->set_multi2_round(dec->b25, opt->round);
	if (code < 0) {
		err = "set_multi2_round failed";
		goto error;
	}

	code = dec->b25->set_strip(dec->b25, opt->strip);
	if (code < 0) {
		err = "set_strip failed";
		goto error;
	}

	code = dec->b25->set_emm_proc(dec->b25, opt->emm);
	if (code < 0) {
		err = "set_emm_proc failed";
		goto error;
	}

	dec->bcas = create_b_cas_card();
	if (!dec->bcas) {
		err = "create_b_cas_card failed";
		goto error;
	}
	code = dec->bcas->init(dec->bcas);
	if (code < 0) {
		err = "bcas->init failed";
		goto error;
	}

	code = dec->b25->set_b_cas_card(dec->b25, dec->bcas);
	if (code < 0) {
		err = "set_b_cas_card failed";
		goto error;
	}

	return dec;

error:
	fprintf(stderr, "%s\n", err);
	free(dec);
	return NULL;
}

void b25_shutdown(DECODER *dec)
{

	if(!dec)
		return;

	if(dec->_data)
		free(dec->_data);
	dec->b25->release(dec->b25);
	dec->bcas->release(dec->bcas);
	free(dec);
}

int b25_decode(DECODER *dec, ARIB_STD_B25_BUFFER *sbuf, ARIB_STD_B25_BUFFER *dbuf)
{
	ARIB_STD_B25_BUFFER buf;
	int code;

	buf.data = sbuf->data;
	buf.size = sbuf->size;
	code = dec->b25->put(dec->b25, &buf);
	if (code < 0) {
		fprintf(stderr, "b25->put failed\n");
		if (code < ARIB_STD_B25_ERROR_NO_ECM_IN_HEAD_32M) {
			uint8_t *p = NULL;
			dec->b25->withdraw(dec->b25, &buf);
			if (buf.size > 0) {
				if (dec->_data != NULL) {
					free(dec->_data);
					dec->_data = NULL;
				}
				p = (uint8_t *)malloc(buf.size + sbuf->size);
			}
			if (p) {
				memcpy(p, buf.data, buf.size);
				memcpy(p + buf.size, sbuf->data, sbuf->size);
				dbuf->data = p;
				dbuf->size = buf.size + sbuf->size;
				dec->_data = p;
				code = 0;
			}
		}
	} else {
		code = dec->b25->get(dec->b25, dbuf);
		if (code < 0)
			fprintf(stderr, "b25->get failed\n");
	}

	return code;
}

int b25_finish(DECODER *dec, ARIB_STD_B25_BUFFER *dbuf)
{
	int code;

	code = dec->b25->flush(dec->b25);
	if (code < 0) {
		fprintf(stderr, "b25->flush failed\n");
		return code;
	}

	code = dec->b25->get(dec->b25, dbuf);
	if (code < 0) {
		fprintf(stderr, "b25->get failed\n");
		return code;
	}

	return code;
}

#else

/* functions */
DECODER *b25_startup(decoder_options *opt)
{
	return NULL;
}

void b25_shutdown(DECODER *dec)
{
}

int b25_decode(DECODER *dec, ARIB_STD_B25_BUFFER *sbuf, ARIB_STD_B25_BUFFER *dbuf)
{
	return 0;
}

int b25_finish(DECODER *dec, ARIB_STD_B25_BUFFER *dbuf)
{
	return 0;
}

#endif
