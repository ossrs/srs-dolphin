.PHONY: default help dolphin clean

SRC=src
OBJS=objs
LINK=g++
CXX=g++
CXXFLAGS = -ansi -Wall -g -O0

default: dolphin

clean:
	@rm -rf $(OBJS)/srs_dolphin $(OBJS)/core $(OBJS)/main

help:
	@echo "Usage: make <help>|<dolphin>"
	@echo "     help        display this help menu"
	@echo "     dolphin     build the srs-dolphin server."
	@echo "For example:"
	@echo "     make"
	@echo "     make dolphin"

dolphin: prepare $(OBJS)/srs_dolphin

prepare:
	@mkdir -p $(OBJS)
	@mkdir -p $(OBJS)/core
	@mkdir -p $(OBJS)/main

ST_LIB=$(OBJS)/st-1.9/obj/libst.a
ST_INCS=-I$(OBJS)/st-1.9/obj

$(OBJS)/srs_dolphin: $(ST_LIB) $(OBJS)/core/dlp_core.o $(OBJS)/main/dlp_main_server.o
	@echo "LINK the srs-dolphin, multiple process SRS."
	$(LINK) -o $(OBJS)/srs_dolphin $(OBJS)/core/dlp_core.o $(OBJS)/main/dlp_main_server.o $(ST_LIB)

$(ST_LIB):
	unzip -q 3rdparty/st-1.9.zip -d objs/
ifeq ($(shell uname -s |grep Darwin > /dev/null 2>&1  && echo yes), yes)
	make -C objs/st-1.9 darwin-debug
else
	make -C objs/st-1.9 linux-debug
endif

CORE_INCS=-I$(SRC)/core $(ST_INCS)
CORE_DEPS=$(SRC)/core/dlp_core.hpp
$(OBJS)/core/dlp_core.o: $(CORE_DEPS) $(SRC)/core/dlp_core.cpp
	$(CXX) -c $(CXXFLAGS) $(CORE_INCS) -o $(OBJS)/core/dlp_core.o $(SRC)/core/dlp_core.cpp

$(OBJS)/main/dlp_main_server.o: $(CORE_DEPS) $(SRC)/main/dlp_main_server.cpp
	$(CXX) -c $(CXXLFAGS) $(CORE_INCS) -o $(OBJS)/main/dlp_main_server.o $(SRC)/main/dlp_main_server.cpp
