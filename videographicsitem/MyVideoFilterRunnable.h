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
const float confidenceThreshold = 0.4f;
const long leftCrop = 500;
const long rightCrop = 130;

// Initialize the parameters
const double confThreshold = 0.6; // Confidence threshold
const float nmsThreshold = 0.4;  // Non-maximum suppression threshold
const int inpWidth = 416;  // Width of network's input image
const int inpHeight = 416; // Height of network's input image

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
    void drawTrackingInfoYOLO(QImage& image);
    cv::Mat QImageToCvMat( const QImage &inImage, bool inCloneImageData = true);

    // Remove the bounding boxes with low confidence using non-maxima suppression
    std::vector<std::tuple<cv::Rect, int, float>> postprocess(cv::Mat& frame, const std::vector<cv::Mat>& out);

    // Get the names of the output layers
    std::vector<std::string> getOutputsNames(const cv::dnn::Net& net);

    float computeIOU(dlib::rectangle boxA, cv::Rect boxB);
protected:
    MyVideoFilter* m_Filter;
    int m_Orientation;
    int m_Flip;
    std::vector<dlib::correlation_tracker> m_trackers;
    std::vector<std::string> m_labels;
    std::vector<float> m_confidence;
    cv::dnn::Net m_net;
    unsigned long m_frameCount;

    std::vector<std::string> m_classes;
};

#endif
