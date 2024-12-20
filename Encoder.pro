QT += core network testlib  xml gui widgets websockets
TARGET = Encoder
CONFIG += c++14

ret = $$find(QMAKESPEC, "v500")
count(ret,1) {
    chip=HI3531D
}

ret = $$find(QMAKESPEC, "mix200_64")
count(ret,1) {
    chip=HI3531DV200
}

ret = $$find(QMAKESPEC, "mix410_64")
count(ret,1) {
    chip=SS626V100
} else {
    ret = $$find(QMAKESPEC, "mix410")
    count(ret,1) {
        chip=SS524V100
    }
}

ret = $$find(QMAKESPEC, "mix210_64")
count(ret,1) {
    chip=SS528V100
}

ret = $$find(QMAKESPEC, "mix100_64")
count(ret,1) {
    chip=HI3559A
    SDKVER=V2.0.3.0
} else {
    ret = $$find(QMAKESPEC, "mix100")
    count(ret,1) {
        chip=HI3516E
    }
}

ret = $$find(QMAKESPEC, "v01c02")
count(ret,1) {
chip=HI3516CV610
}


include(/home/linkpi/work/LinkLib/Link.pri)
LIBS +=-lLinkUI
if(!contains(DEFINES,HI3516E)){
if(!contains(DEFINES,HI3516CV610)){
    include(/home/linkpi/work/LinkLib/LinkNDI.pri)
    include(/home/linkpi/work/LinkLib/LinkGB28181.pri)
}
}

TEMPLATE = app

MOC_DIR = Temp/$$chip
OBJECTS_DIR  = Temp/$$chip

SOURCES += main.cpp \
    GB28181.cpp \
    RPC.cpp \
    Group.cpp \
    Channel.cpp \
    ChannelNet.cpp \
    ChannelVI.cpp \
    Config.cpp \
    GroupRPC.cpp \
    ChannelMix.cpp \
    ChannelUSB.cpp \
    ChannelFile.cpp \
    Synchronization.cpp \
    jcon/string_util.cpp \
    jcon/json_rpc_debug_logger.cpp \
    jcon/json_rpc_endpoint.cpp \
    jcon/json_rpc_error.cpp \
    jcon/json_rpc_file_logger.cpp \
    jcon/json_rpc_logger.cpp \
    jcon/json_rpc_request.cpp \
    jcon/json_rpc_server.cpp \
    jcon/json_rpc_success.cpp \
    jcon/json_rpc_client.cpp \
    jcon/json_rpc_tcp_client.cpp \
    jcon/json_rpc_tcp_server.cpp \
    jcon/json_rpc_tcp_socket.cpp \
    Record.cpp \
    Push.cpp \
    UART.cpp \
    ChannelNDI.cpp \
    ChannelAlsa.cpp \
    Intercom.cpp \
    ChannelColorKey.cpp \
    oled/Gpio.cpp \
    oled/Oled.cpp \
    oled/Plug/Recd.cpp \
    oled/Home.cpp \
    oled/Plug/View.cpp \
    oled/Plug.cpp \
    oled/Plug/PUSH.cpp \
    oled/Home/Dash.cpp \
    oled/Home/Default.cpp \
    oled/Home/Network.cpp \
    oled/Home/Netward.cpp \
    oled/Home/Report.cpp \
    oled/Plug/NetEth.cpp \
    oled/Plug/NetWifi.cpp \
    oled/Notify.cpp \
    LED.cpp \
    Capture.cpp

HEADERS += \
    GB28181.h \
    RPC.h \
    Group.h \
    Channel.h \
    ChannelNet.h \
    ChannelVI.h \
    Config.h \
    GroupRPC.h \
    ChannelMix.h \
    ChannelUSB.h \
    ChannelFile.h \
    Synchronization.h \
    jcon/string_util.h \
    jcon/json_rpc_debug_logger.h \
    jcon/json_rpc_endpoint.h \
    jcon/json_rpc_error.h \
    jcon/json_rpc_file_logger.h \
    jcon/json_rpc_logger.h \
    jcon/json_rpc_request.h \
    jcon/json_rpc_result.h \
    jcon/json_rpc_server.h \
    jcon/json_rpc_socket.h \
    jcon/json_rpc_success.h \
    jcon/jcon_assert.h \
    jcon/jcon.h \
    jcon/json_rpc_client.h \
    jcon/json_rpc_tcp_client.h \
    jcon/json_rpc_tcp_server.h \
    jcon/json_rpc_tcp_socket.h \
    Version.h \
    Record.h \
    Push.h \
    UART.h \
    ChannelNDI.h \
    ChannelAlsa.h \
    Intercom.h \
    ChannelColorKey.h \
    oled/Gpio.h \
    oled/Oled.h \
    oled/Regist.h \
    oled/Plug/Recd.h \
    oled/Home.h \
    oled/Plug/View.h \
    oled/Plug.h \
    oled/Plug/PUSH.h \
    oled/Home/Dash.h \
    oled/Home/Default.h \
    oled/Home/Network.h \
    oled/Home/Netward.h \
    oled/Home/Report.h \
    oled/Plug/NetEth.h \
    oled/Plug/NetWifi.h \
    oled/Notify.h \
    LED.h \
    Capture.h

FORMS += \
    oled/Dash.ui \
    oled/View.ui \
    oled/Notify.ui

RESOURCES += \
    res.qrc
