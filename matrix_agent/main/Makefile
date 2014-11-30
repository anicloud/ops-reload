# Copyright 2010 Yiyuan Zhong, Shanghai Jiao Tong University. All Rights Reserved.
# Author: yiyuanzhong@gmail.com (Yiyuan Zhong)

### MODIFY ANYTHING BELOW THIS LINE THAT IS SUITABLE ###

TARGET = ../bin/matrix

SUBDIRS = .

# "d" will be appended for each STATICS, ABC will become -lABCd eventually
STATICS = inf/computing/matrix/matrix-agent/lib/matrix \
          op/oped/noah/sailor/protocol/output/lib/sailor_protocol \
          op/oped/noah/sailor/utility/output/lib/sailor_utility

LIBRARIES = thrift protobuf pthread rt \
            ullib config bsl spreg boost_thread

LDFLAGS += -Wl,-Bstatic -luuid -Wl,-Bdynamic -ldl

MODULEROOT = ../../../../..
MODULEPATH = op/oped/matrix/agent

BINPATH = op/oped/noah/thirdparty/thrift/output/bin

SEARCHPATH = \
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

LIBRARYPATH = \
    op/oped/noah/thirdparty/thrift/output/lib \
    public/configure/output/lib \
    com/btest/gtest/output/lib \
    public/spreg/output/lib \
    $(THIRD)/protobuf/lib \
    $(THIRD)/boost/lib \
    $(LIB2)/ullib/lib \
    $(LIB2)/bsl/lib

# Installing related
DESTDIR =
prefix = /usr
EXTRA_INSTALLS =

### MODIFY ANYTHING BELOW THIS LINE CAREFULLY ###

CROSS_COMPILE =

### DON'T MODIFY ANYTHING BELOW THIS LINE ###

GOAL = $(EXECUTABLE)
GOALd = $(EXECUTABLEd)

include $(MODULEROOT)/op/oped/noah/sailor/utility/include.mk
