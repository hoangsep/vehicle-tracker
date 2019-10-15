TEMPLATE = app
TARGET = videographicsitem

QT += qml quick multimedia multimediawidgets

HEADERS   += \
    MyVideoFilter.h \
    MyVideoFilterRunnable.h

INCLUDEPATH += /usr/local/include/opencv4
LIBS += $(shell pkg-config opencv --libs)
LIBS += -L /usr/local/lib -lopencv_core -lopencv_imgproc -lopencv_highgui -lopencv_videoio -lopencv_imgcodecs -ldlib -lopencv_dnn -lblas -llapack
QMAKE_CXXFLAGS += -std=c++11
PKGCONFIG += dlib-1

SOURCES   += main.cpp \
             MyVideoFilter.cpp \
             MyVideoFilterRunnable.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/multimediawidgets/videographicsitem
INSTALLS += target

QT+=widgets

DISTFILES += \
    SSD_300x300/VGG_coco_SSD_300x300_iter_400000.caffemodel \
    SSD_300x300/deploy.prototxt \
    SSD_300x300/score_ssd_coco_minival.py \
    SSD_300x300/score_ssd_coco_testdev.py \
    SSD_300x300/solver.prototxt \
    SSD_300x300/ssd_coco.py \
    SSD_300x300/test.prototxt \
    SSD_300x300/train.prototxt \
    SSD_300x300_coco/VGG_coco_SSD_300x300.caffemodel \
    SSD_300x300_coco/deploy.prototxt \
    YOLO/coco.names \
    YOLO/yolov3.cfg \
    YOLO/yolov3.weights \
    mobilenet_ssd/MobileNetSSD_deploy.caffemodel \
    mobilenet_ssd/MobileNetSSD_deploy.prototxt

RESOURCES += \
    qml.qrc

