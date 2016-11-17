MODULE_big = pg_fuck_block
OBJS = pg_fuck_block.o $(WIN32RES)

EXTENSION = pg_fuck_block
DATA = pg_fuck_block--1.0.sql 
PGFILEDESC = "pg_fuck_block - get BlockNumber all Tuple"
USE_PGXS=1



ifdef USE_PGXS
PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
else
subdir = contrib/pg_fuck_block
top_builddir = ../..
include $(top_builddir)/src/Makefile.global
include $(top_srcdir)/contrib/contrib-global.mk
endif

