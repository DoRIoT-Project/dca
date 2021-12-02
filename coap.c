/*
 * Copyright (C) 2021 Otto-von-Guericke-Universität Magdeburg
 */

 /**
  * @author  Frank Engelhardt <fengelha@ovgu.de>
  */
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "doriot_dca.h"
#include "net/gcoap.h"
#include "od.h"
#include "fmt.h"

#define ENABLE_DEBUG 0
#include "debug.h"

#define DCA_COAP_STRBUF_SIZE 128

static ssize_t _encode_link(const coap_resource_t *resource, char *buf,
                            size_t maxlen, coap_link_encoder_ctx_t *context);
static ssize_t _dca_handler(coap_pkt_t* pdu, uint8_t *buf, size_t len, void *ctx);

/* CoAP resources. Must be sorted by path (ASCII order). */
static const coap_resource_t _resources[] = {
    { "/dca", COAP_GET | COAP_MATCH_SUBTREE, _dca_handler, NULL },
};

static const char *_link_params[] = {
    ";ct=0;rt=\"dca\"",
    NULL
};

static gcoap_listener_t _listener = {
    &_resources[0],
    ARRAY_SIZE(_resources),
    _encode_link,
    NULL,
    NULL
};

/* Adds link format params to resource list */
static ssize_t _encode_link(const coap_resource_t *resource, char *buf,
                            size_t maxlen, coap_link_encoder_ctx_t *context) {
    ssize_t res = gcoap_encode_link(resource, buf, maxlen, context);
    if (res > 0) {
        if (_link_params[context->link_pos]
                && (strlen(_link_params[context->link_pos]) < (maxlen - res))) {
            if (buf) {
                memcpy(buf+res, _link_params[context->link_pos],
                       strlen(_link_params[context->link_pos]));
            }
            return res + strlen(_link_params[context->link_pos]);
        }
    }

    return res;
}


static ssize_t _dca_handler(coap_pkt_t* pdu, uint8_t *buf, size_t len, void *ctx)
{
    (void)ctx;
    char uripath[CONFIG_NANOCOAP_URI_MAX];
    char *dbpath;

    /* get URI path and fetch the */
    size_t uripathlen = coap_get_uri_path(pdu, (uint8_t*)uripath);
    if(uripathlen <= 4) {
        DEBUG("invalid requst: %s\n", uripath);
        return gcoap_response(pdu, buf, len, COAP_CODE_404);
    }
    dbpath = uripath + 4;

    /* fetch db entry */
    db_node_t node;
    int r = db_find_node_by_path(dbpath, &node);
    if(r < 0) {
        DEBUG("invalid requst: %s\n", uripath);
        return gcoap_response(pdu, buf, len, COAP_CODE_404);
    }
    char val[DCA_COAP_STRBUF_SIZE];
    val[0] = '\0';
    size_t vallen = 0;
    db_node_type_t type = db_node_get_type(&node);

    if (type == db_node_type_inner) {
        /* print a list of all child nodes */
        db_node_t child;
        db_node_get_next_child(&node, &child);
        char name[DB_NODE_NAME_MAX];
        while (!db_node_is_null(&child)) {
            db_node_get_name(&child, name);
            strncat(val, name, DCA_COAP_STRBUF_SIZE-1);
            strncat(val, " ", DCA_COAP_STRBUF_SIZE-1);
            db_node_get_next_child(&node, &child);
        }
 		vallen = strlen(val);
    }
    else if (type == db_node_type_int
             || type == db_node_type_float
             || type == db_node_type_str) {
        r = db_node_value_to_str(&node, val, DCA_COAP_STRBUF_SIZE);
        if(r < 0) {
            /* should not happen */
            return gcoap_response(pdu, buf, len, COAP_CODE_INTERNAL_SERVER_ERROR);
        }
        vallen = (size_t)r;
    }

    /* build and send response */
    gcoap_resp_init(pdu, buf, len, COAP_CODE_CONTENT);
    coap_opt_add_format(pdu, COAP_FORMAT_TEXT);
    size_t resp_len = coap_opt_finish(pdu, COAP_OPT_FINISH_PAYLOAD);

    if (pdu->payload_len >= vallen) {
        memcpy(pdu->payload, val, vallen);
        return resp_len + vallen;
    }
    else {
        DEBUG("gcoap_cli: msg buffer too small\n");
        return gcoap_response(pdu, buf, len, COAP_CODE_INTERNAL_SERVER_ERROR);
    }
}

int db_coap_init(void)
{
    gcoap_register_listener(&_listener);
    return 0;
}
