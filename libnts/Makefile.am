AM_CPPFLAGS = -I$(srcdir)/include

libntslibdir = $(libdir)

ACLOCAL_AMFLAGS = -I config -I m4
AM_CFLAGS = -g -Wall -std=c99

SUBDIRS = lib src include tests examples
EXTRA_DIST = autogen.sh

# AUTOMAKE_OPTIONS 是Automake 的选项。Automake 主要是帮助开发 GNU 软
# 件的人员来维护软件，所以在执行Automake 时，会检查目录下是否存在标准 GNU 软件中
# 应具备的文件，例如 'NEWS'、'AUTHOR'、 'ChangeLog' 等文件。设置为foreign 时，Automake
# 会改用一般软件的标准来检查。如果不加这句的话，需要在autoconf之前，先执行touch NEWS
# README AUTHORS ChangeLog 来生成'NEWS'、'AUTHOR'、 'ChangeLog' 等文件
AUTOMAKE_OPTIONS=foreign
