CONFIG -= qt

TEMPLATE = lib
DEFINES += NTB_LIBRARY

CONFIG += c++11

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    ntb.cpp \
    ntb_errors.cpp \
    ntb_log.cpp \
    ntb_signals.cpp \
    ntb_ss_client.cpp \
    ntb_ss_client_in.cpp \
    ntb_ss_client_init.cpp \
    ntb_ss_client_out.cpp \
    ntb_ssl.cpp \
    ntb_timers.cpp

HEADERS += \
    NTB_global.h \
    ntb.h \
    ntb_charrp.h \
    ntb_colors.h \
    ntb_errors.h \
    ntb_log.h \
    ntb_mempool.h \
    ntb_pdllist.h \
    ntb_signals.h \
    ntb_spinlock.h \
    ntb_ss_client.h \
    ntb_ss_core.h \
    ntb_ssl.h \
    ntb_timers.h \
    ntb_util.h

# Default rules for deployment.
unix {
    target.path = /usr/lib
}
!isEmpty(target.path): INSTALLS += target
