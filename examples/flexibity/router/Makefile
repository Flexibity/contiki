CONTIKI_PROJECT=border-router
all: $(CONTIKI_PROJECT) 

CONTIKI=../../..

WITH_UIP6=1
UIP_CONF_IPV6=1
CFLAGS+= -DUIP_CONF_IPV6_RPL

#linker optimizations
SMALL=1

CFLAGS += -DPROJECT_CONF_H=\"project-conf.h\"
PROJECT_SOURCEFILES += slip-bridge.c

CFLAGS += -DWEBSERVER=1
PROJECT_SOURCEFILES += httpd-simple.c

include $(CONTIKI)/Makefile.include

