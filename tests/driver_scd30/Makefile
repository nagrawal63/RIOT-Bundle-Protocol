
BOARD ?= nucleo-l073rz
include ../Makefile.tests_common


### Modules

# base
USEMODULE += xtimer
USEMODULE += printf_float

# shell
USEMODULE += shell
USEMODULE += shell_commands

### SAUL
USEMODULE += auto_init_saul
#USEMODULE += saul_default
# scd30
FEATURES_REQUIRED = periph_i2c
USEMODULE += scd30


#CFLAGS += -DSCD30_EXT_SAUL=1

### OS
include $(RIOTBASE)/Makefile.include
