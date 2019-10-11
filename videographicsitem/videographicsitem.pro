TEMPLATE = app
TARGET = videographicsitem

QT += multimedia multimediawidgets

HEADERS   += videoplayer.h

INCLUDEPATH += /usr/local/include/opencv4
LIBS += $(shell pkg-config opencv --libs)
LIBS += -lopencv_dnn
#LIBS += -L /usr/local/lib -lopencv_core -lopencv_imgproc -lopencv_highgui -lopencv_videoio -lopencv_imgcodecs -ldlib  -lopencv_dnn
QMAKE_CXXFLAGS += -std=c++11
PKGCONFIG += dlib-1

SOURCES   += main.cpp \
             videoplayer.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/multimediawidgets/videographicsitem
INSTALLS += target

QT+=widgets

DISTFILES += \
    mobilenet_ssd/MobileNetSSD_deploy.caffemodel \
    mobilenet_ssd/MobileNetSSD_deploy.prototxt

