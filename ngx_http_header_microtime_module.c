/**	Add a header containing the current epoch with microseconds.
	Implemented solely because there exists a registered RFC5785
	service, defined by http://phk.freebsd.dk/time/20151129.html
*/

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>

#define DEFAULT_HEADER_NAME "X-HTTPSTIME"

static const char * const module_name_ = "header_microtime_module";


typedef struct {
	ngx_flag_t enabled;
	ngx_str_t name;
} ngx_http_header_microtime_loc_conf_t;


static ngx_http_output_header_filter_pt ngx_http_next_header_filter_;


static ngx_int_t ngx_http_header_microtime_init_(ngx_conf_t *);
static void *ngx_http_http_header_microtime_create_loc_conf_(ngx_conf_t *);
static char *ngx_http_http_header_microtime_merge_loc_conf_(ngx_conf_t *, void *, void *);
static ngx_int_t ngx_http_header_microtime_header_filter_(ngx_http_request_t *);


static ngx_command_t ngx_http_header_microtime_commands_[] = {
	{	ngx_string("header_microtime"),
		NGX_HTTP_LOC_CONF|NGX_CONF_FLAG,
		ngx_conf_set_flag_slot,
		NGX_HTTP_LOC_CONF_OFFSET,
		offsetof(ngx_http_header_microtime_loc_conf_t, enabled),
		NULL
	},

	{	ngx_string("header_microtime_name"),
		NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
		ngx_conf_set_str_slot,
		NGX_HTTP_LOC_CONF_OFFSET,
		offsetof(ngx_http_header_microtime_loc_conf_t, name),
		NULL
	},

	ngx_null_command
};


static ngx_http_module_t ngx_http_header_microtime_module_ctx_ = {
	NULL,
	ngx_http_header_microtime_init_,
	NULL,
	NULL,
	NULL,
	NULL,
	ngx_http_http_header_microtime_create_loc_conf_,
	ngx_http_http_header_microtime_merge_loc_conf_
};


static
ngx_int_t ngx_http_header_microtime_init_(ngx_conf_t *cf) {
	(void)cf;

	ngx_http_next_header_filter_ = ngx_http_top_header_filter;
	ngx_http_top_header_filter = ngx_http_header_microtime_header_filter_;

	return NGX_OK;
}

static
void *ngx_http_http_header_microtime_create_loc_conf_(ngx_conf_t *cf) {
	ngx_http_header_microtime_loc_conf_t *conf;
	conf = ngx_pcalloc(cf->pool, sizeof *conf);
	if (conf == NULL)
		return NGX_CONF_ERROR;
	conf->enabled = NGX_CONF_UNSET;

	return conf;
}


static
char *ngx_http_http_header_microtime_merge_loc_conf_(ngx_conf_t *cf, void *parent, void *child) {
	ngx_http_header_microtime_loc_conf_t *prev = parent;
	ngx_http_header_microtime_loc_conf_t *conf = child;

	ngx_conf_merge_value(conf->enabled, prev->enabled, 0);
	ngx_conf_merge_str_value(conf->name, prev->name, DEFAULT_HEADER_NAME);

	if (conf->enabled != 0 && conf->enabled != 1) {
		ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "\"%s\" ought to be \"on\" or \"off\", not \"%d\"", "header_microtime", conf->enabled);
		return NGX_CONF_ERROR;
	}

	return NGX_CONF_OK;
}


ngx_module_t ngx_http_header_microtime_module = {
	NGX_MODULE_V1,
	&ngx_http_header_microtime_module_ctx_,
	ngx_http_header_microtime_commands_,
	NGX_HTTP_MODULE,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NGX_MODULE_V1_PADDING
};


static
ngx_int_t ngx_http_header_microtime_header_filter_(ngx_http_request_t *r) {
	ngx_http_header_microtime_loc_conf_t *loc_conf = ngx_http_get_module_loc_conf(r, ngx_http_header_microtime_module);

	if (loc_conf->enabled == 1) {
		struct timeval tv;
		ngx_keyval_t hkv;
		ngx_table_elt_t *h;
		int l;

		hkv.key = loc_conf->name;

		hkv.value.len = 1 + sizeof "18446744073709551614.999999";
		if ((hkv.value.data = ngx_pnalloc(r->pool, hkv.value.len)) == NULL) {
			return NGX_HTTP_INTERNAL_SERVER_ERROR;
		}

		h = ngx_list_push(&r->headers_out.headers);
		if (h == NULL) {
			return NGX_HTTP_INTERNAL_SERVER_ERROR;
		}

		/* for now, not worrying about the cost of calling this on each request */
		if (gettimeofday(&tv, NULL) != 0) {
			ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
			              "%s gettimeofday: %s",
			              module_name_, strerror(errno));
			return NGX_HTTP_INTERNAL_SERVER_ERROR;
		}
		l = snprintf((char *)hkv.value.data, hkv.value.len,
		             "%ld.%06ld",
		             (long)tv.tv_sec, (long)tv.tv_usec);
		if (l < 0
		||  (size_t)l > hkv.value.len) {
			ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
			              "%s snprintf returned %d on len %zu",
			              module_name_, l, hkv.value.len);
			return NGX_HTTP_INTERNAL_SERVER_ERROR;
		}
		hkv.value.len = l;

		h->hash = 1;
		h->key = hkv.key;
		h->value = hkv.value;

		ngx_log_debug(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
		              "%s done", module_name_);
	}

	return ngx_http_next_header_filter_(r);
}
