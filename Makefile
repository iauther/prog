
NAME=prog

CUR_DIR = $(shell pwd)
SRC_DIR = $(CUR_DIR)


BIN_DIR = /usr/bin
CC = $(BIN_DIR)/gcc
LD = $(BIN_DIR)/ld
AS = $(BIN_DIR)/as
AR = $(BIN_DIR)/ar
STRIP = $(BIN_DIR)/strip

         
srcDIR  = $(SRC_DIR)
bldDIR  = $(SRC_DIR)/build

incDIRS +=  inc
libDIRS +=  lib

allSRCS +=  src/ch341a.c \
            src/sf.c \
            src/main.c
   
               
               
DFLAGS  += -D__USE_GNU -D_GNU_SOURCE #-D_POSIX_SOURCE

allINCS = $(addprefix $(srcDIR)/,$(incDIRS)) $(usrINC)
allOBJS = $(addprefix $(bldDIR)/,$(allSRCS:.c=.o))
allDEPS = $(allOBJS:.o=.d)


######################################################################

GDB_FLAGS = -g -O0 #-v -da -Q 

depLIBS  = -lrt -ldl -pthread -L$(libDIRS) -lusb-1.0
libFLAGS = -Wl,-rpath,'$$ORIGIN'

IFLAGS   += $(addprefix -I,$(allINCS))
CFLAGS   += $(GDB_FLAGS) -w -rdynamic -std=c99 $(SYS_ROOT) $(IFLAGS) $(DFLAGS)
DEPFLAGS += -MMD -MP -MF $(@:.o=.d)
LDFLAGS += -s $(depLIBS) $(libFLAGS)


.PHONEY: all clean lint

TARGET = $(srcDIR)/$(NAME)

all: $(TARGET)


DEMO_DIR =/home/gh/Desktop/demo/widora

$(TARGET): $(allOBJS)
	@$(CC) $(LDFLAGS) $^ -o $@
	-cp -uf $@ ../files/kb/
	-cp -uf $@ $(DEMO_DIR)/


$(bldDIR)/%.o:$(srcDIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) -c $(CFLAGS) $(DEPFLAGS) $< -o $@

clean:
	@rm -fr $(bldDIR) $(TARGET)


allC = $(addprefix $(srcDIR)/,$(allSRCS))
lint:
	splint $(allC)

-include $(allDEPS)




