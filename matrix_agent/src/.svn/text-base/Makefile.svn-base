# Copyright 2010 Yiyuan Zhong, Shanghai Jiao Tong University. All Rights Reserved.
# Author: yiyuanzhong@gmail.com (Yiyuan Zhong)

### MODIFY ANYTHING BELOW THIS LINE THAT IS SUITABLE ###

TARGET = ../lib/matrix

SUBDIRS = ../common ../executor ../manager ../monitor ../protocol ../reporter ../rpc ../json-cpp ../httpd .. .

MODULEROOT = ../../../../..
MODULEPATH = inf/computing/matrix/matrix-agent

BINPATH = op/oped/noah/thirdparty/thrift/output/bin

SEARCHPATH = \
    inf/computing/matrix/matrix-agent/json-cpp/include \
    op/oped/noah/thirdparty/thrift/output/include \
    op/oped/noah/sailor/protocol/output/include \
    op/oped/noah/sailor/utility/output/include \
    public/configure/output/include \
    com/btest/gtest/output/include \
    public/spreg/output/include \
    $(THIRD)/protobuf/include \
    $(THIRD)/boost/include \
    $(LIB2)/ullib/include \
    $(LIB2)/bsl/include

# Installing related
DESTDIR =
prefix = /usr
EXTRA_INSTALLS =

### MODIFY ANYTHING BELOW THIS LINE CAREFULLY ###

CROSS_COMPILE =

### DON'T MODIFY ANYTHING BELOW THIS LINE ###

GOAL = $(LIBRARY)
GOALd = $(LIBRARYd)

include $(MODULEROOT)/op/oped/noah/sailor/utility/include.mk
