#target names
EXE_DEBUG = pushsvr.d

all:$(EXE_DEBUG)

#include file
ROOT = .
CPP = $(wildcard $(ROOT)/*.cpp)
CPP_DATA = $(wildcard $(ROOT)/dataParser/*.cpp)
CPP_UNIT = $(wildcard $(ROOT)/unit/*.cpp)
C_UNIT = $(wildcard $(ROOT)/unit/*.c)
C_MIO = $(wildcard $(ROOT)/mio/*.c)
C_MIO2 = $(wildcard $(ROOT)/mio2/*.cpp)
#C_ZIP = $(wildcard $(ROOT)/zip/*.c)

#obj file
FTSOBJS = $(patsubst %.cpp,%.o,$(CPP)) $(patsubst %.cpp,%.o,$(CPP_DATA)) $(patsubst %.cpp,%.o,$(CPP_UNIT)) $(patsubst %.c,%.o,$(C_UNIT)) $(patsubst %.c,%.o,$(C_MIO)) $(patsubst %.cpp,%.o,$(C_MIO2))

#sql lib path
LDFLAGSSQL = -L/usr/local/mysql/lib -L$$(ROOT)/ -L$(ROOT)/libs

#include directories
#INCDIRS = -I$(ROOT)/ -I$(ROOT)/dataParser -I$(ROOT)/mio -I$(ROOT)/mio2 -I$(ROOT)/unit -I$(ROOT)/zip -I/usr/local/mysql/include
INCDIRS = -I$(ROOT)/ -I$(ROOT)/dataParser -I$(ROOT)/mio -I$(ROOT)/mio2 -I$(ROOT)/unit -I/usr/local/mysql/include

#compile rule
CC = g++
CFLAGS = -c -g
LINKFLAGS = -lnsl -lpthread -lz -lmysqlclient -ljsonlib

COMPILE = $(CC) $(INCDIRS) $(CFLAGS) $(@D)/$(<F) -o $(@D)/$(@F)
LINK = $(CC) $(INCDIRS) $(LDFLAGSSQL) -o $(EXE_DEBUG) $(FTSOBJS) $(LINKFLAGS)

%.o:%.cpp
	$(COMPILE)
%.o:%.c
	$(COMPILE)

$(EXE_DEBUG): $(FTSOBJS)
	$(LINK)

clear:
	@rm -f $(FTSOBJS) $(EXE_DEBUG)

