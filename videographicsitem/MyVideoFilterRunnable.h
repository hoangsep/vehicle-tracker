#ifndef MyVideoFilterRunnable_H
#define MyVideoFilterRunnable_H

#include <QVideoFilterRunnable>
#include <QVideoFrame>
#include <QPainter>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/dnn.hpp>
#include <opencv2/videoio.hpp>

#include <dlib/image_processing.h>
#include <dlib/gui_widgets.h>
#include <dlib/image_io.h>
#include <dlib/dir_nav.h>

// initialize the list of class labels MobileNet SSD was trained to
// detect
static std::string CLASSES[] = {"background", "aeroplane", "bicycle", "bird", "boat",
    "bottle", "bus", "car", "cat", "chair", "cow", "diningtable",
    "dog", "horse", "motorbike", "person", "pottedplant", "sheep",
    "sofa", "train", "tvmonitor"};

class MyVideoFilter;

class MyVideoFilterRunnable : public QVideoFilterRunnable {
public:
    MyVideoFilterRunnable(MyVideoFilter* parent = nullptr);

    QVideoFrame run(QVideoFrame *input, const QVideoSurfaceFormat &surfaceFormat, RunFlags flags) Q_DECL_OVERRIDE;

    static QImage QVideoFrameToQImage(QVideoFrame* input);
    static QImage QVideoFrameToQImage_using_Qt_internals(QVideoFrame* input);
    static QImage QVideoFrameToQImage_using_GLTextureHandle(QVideoFrame* input);
    static void drawRedGreenPixels(QImage& image);
    void drawTrackingInfo(QImage& image);
    cv::Mat QImageToCvMat( const QImage &inImage, bool inCloneImageData = true);
//    void QImageToCvMat(const QImage& image, cv::OutputArray out);

protected:
    MyVideoFilter* m_Filter;
    int m_Orientation;
    int m_Flip;
    dlib::correlation_tracker tracker;
//    printf("[INFO] loading model...");
    cv::dnn::Net net;
//    print("[INFO] starting video stream...");
};

#endif
