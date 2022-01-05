BOARDNAME := stm32g0-nucleo64

ENABLE_NET_MBUS := yes
ENABLE_NET_PCS := yes

B := ${SRC}/platform/${BOARDNAME}

GLOBALDEPS += ${B}/${BOARDNAME}.mk

CPPFLAGS += -I${B} -include ${BOARDNAME}.h

include ${SRC}/platform/stm32g0/stm32g0.mk

SRCS += ${B}/${BOARDNAME}.c
