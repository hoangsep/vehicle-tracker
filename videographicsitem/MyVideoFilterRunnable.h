#ifndef MyVideoFilterRunnable_H
#define MyVideoFilterRunnable_H

#include <QVideoFilterRunnable>
#include <QVideoFrame>
#include <QPainter>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/dnn.hpp>

#include <dlib/image_processing.h>
#include <dlib/opencv/cv_image.h>

// initialize the list of class labels MobileNet SSD was trained to detect
static std::string CLASSES[] = {"background", "aeroplane", "bicycle", "bird", "boat",
    "bottle", "bus", "car", "cat", "chair", "cow", "diningtable",
    "dog", "horse", "motorbike", "person", "pottedplant", "sheep",
    "sofa", "train", "tvmonitor"};

const float confidenceThreshold = 0.4f;
const long leftCrop = 500;
const long rightCrop = 130;
const float confThreshold = 0.6f; // Confidence threshold
const float nmsThreshold = 0.4f;  // Non-maximum suppression threshold
const int inpWidth = 416;  // Width of network's input image
const int inpHeight = 416; // Height of network's input image

const int objectDetectionPeriod = 20;   // detect object each 20 frames
const int speedTrackPeriod = 20;        // track object speed each 20 frames
const double moveDistThreshold = 80;    // threshold to quantify if object is moving

class MyVideoFilter;

/**
 * @brief The MyVideoFilterRunnable class
 */
class MyVideoFilterRunnable : public QVideoFilterRunnable {
public:
    /**
     * @brief MyVideoFilterRunnable
     * @param parent
     */
    MyVideoFilterRunnable(MyVideoFilter* parent = nullptr);

    /**
     * @brief run
     * @param input
     * @param surfaceFormat
     * @param flags
     * @return
     */
    QVideoFrame run(QVideoFrame *input, const QVideoSurfaceFormat &surfaceFormat, RunFlags flags) Q_DECL_OVERRIDE;

    /**
     * @brief QVideoFrameToQImage
     * @param input
     * @return
     */
    static QImage QVideoFrameToQImage(QVideoFrame* input);

    /**
     * @brief QVideoFrameToQImage_using_Qt_internals
     * @param input
     * @return
     */
    static QImage QVideoFrameToQImage_using_Qt_internals(QVideoFrame* input);

    /**
     * @brief QVideoFrameToQImage_using_GLTextureHandle
     * @param input
     * @return
     */
    static QImage QVideoFrameToQImage_using_GLTextureHandle(QVideoFrame* input);

    /**
     * @brief drawTrackingInfo
     * @param image
     */
    void drawTrackingInfo(QImage& image);

    /**
     * @brief drawTrackingInfoYOLO
     * @param image
     */
    void drawTrackingInfoYOLO(QImage& image);

    /**
     * @brief QImageToCvMat
     * @param inImage
     * @param inCloneImageData
     * @return
     */
    cv::Mat QImageToCvMat( const QImage &inImage, bool inCloneImageData = true);

    // Remove the bounding boxes with low confidence using non-maxima suppression
    /**
     * @brief postprocess
     * @param frame
     * @param out
     * @return
     */
    std::vector<std::tuple<cv::Rect, int, float>> postprocess(cv::Mat& frame, const std::vector<cv::Mat>& out);

    /**
     * @brief getOutputsNames Get the names of the output layers
     * @param net
     * @return
     */
    std::vector<std::string> getOutputsNames(const cv::dnn::Net& net);

    /**
     * @brief compute intersection over union of a dlib rectange and
     * an opencv Rect
     * @param boxA a dlib rectange
     * @param boxB an opencv Rect
     * @return Intersection over union of boxA and boxB
     */
    float computeIOU(dlib::rectangle boxA, cv::Rect boxB);

protected:
    MyVideoFilter* m_Filter;
    int m_Orientation;
    int m_Flip;

    // This 5 vectors go together and the same index in each vector
    // will refer to the same tracker
    // TODO: Make this into a class
    std::vector<dlib::correlation_tracker> m_trackers;  // dlib tracker used to track trucks
    std::vector<std::string> m_labels;                  // label of the tracker above
    std::vector<float> m_confidences;                   // confidence of tracker above
    std::vector<std::pair<long, long>> m_centroids;     // x and y of centroid of bounding box
    std::vector<unsigned long> m_startFrame;            // start frame this truck has stopped

    cv::dnn::Net m_net;                 // the network used for detection
    unsigned long m_frameCount;         // number of frame passed
    std::vector<std::string> m_classes; // classes in object detector
    double m_frameRate;                 // frame rate of the video
};

#endif
