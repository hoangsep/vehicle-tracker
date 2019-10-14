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
#include <dlib/opencv/cv_image.h>

// initialize the list of class labels MobileNet SSD was trained to
// detect
static std::string CLASSES[] = {"background", "aeroplane", "bicycle", "bird", "boat",
    "bottle", "bus", "car", "cat", "chair", "cow", "diningtable",
    "dog", "horse", "motorbike", "person", "pottedplant", "sheep",
    "sofa", "train", "tvmonitor"};
const float confidenceThreshold = 0.3f;

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

protected:
    MyVideoFilter* m_Filter;
    int m_Orientation;
    int m_Flip;
    std::vector<dlib::correlation_tracker> m_trackers;
    std::vector<std::string> m_labels;
    cv::dnn::Net m_net;
    unsigned long m_frameCount;
};

#endif
