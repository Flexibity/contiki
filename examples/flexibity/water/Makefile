all: water

ifndef TARGET
TARGET=flexibity
endif

CONTIKI=../../..

WITH_UIP6=1
UIP_CONF_IPV6=1

WITH_COAP = 0

CFLAGS += -DPROJECT_CONF_H=\"project-conf.h\"

ifeq ($(WITH_COAP), 1)
CFLAGS += -DWITH_COAP
APPS += rest-coap
else
CFLAGS_OPT= -Os
CFLAGS_DEBUG=-ggdb
CFLAGS += -DWITH_HTTP
APPS += rest-http
endif

include $(CONTIKI)/Makefile.include

