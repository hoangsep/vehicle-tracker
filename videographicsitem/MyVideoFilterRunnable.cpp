#include "MyVideoFilterRunnable.h"

#include <QDebug>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QQmlContext>
#include <QDateTime>
#include "MyVideoFilter.h"

QImage qt_imageFromVideoFrame(const QVideoFrame& f);

MyVideoFilterRunnable::MyVideoFilterRunnable(MyVideoFilter* parent) :
    m_Filter(parent),
    m_Orientation(0),
    m_Flip(0),
    m_frameCount(0) {
    std::string prototxt1 = "/home/mihota/vision/vehicle-tracker/videographicsitem/mobilenet_ssd/MobileNetSSD_deploy.prototxt";
    std::string model1 = "/home/mihota/vision/vehicle-tracker/videographicsitem/mobilenet_ssd/MobileNetSSD_deploy.caffemodel";

    std::string prototxt2 = "/home/mihota/vision/vehicle-tracker/videographicsitem/SSD_300x300_coco/deploy.prototxt";
    std::string model2 = "/home/mihota/vision/vehicle-tracker/videographicsitem/SSD_300x300_coco/VGG_coco_SSD_300x300.caffemodel";

    std::string prototxt3 = "/home/mihota/vision/vehicle-tracker/videographicsitem/SSD_300x300/deploy.prototxt";
    std::string model3 = "/home/mihota/vision/vehicle-tracker/videographicsitem/SSD_300x300/VGG_coco_SSD_300x300_iter_400000.caffemodel";

//    m_net = cv::dnn::readNetFromCaffe(prototxt1, model1);
    // YOLO time --------------------------------------------------------------------
    // Load names of classes
    std::string folderName = "/home/mihota/vision/vehicle-tracker/videographicsitem/YOLO/";
    std::string classesFile = folderName + "coco.names";
    std::ifstream ifs(classesFile.c_str());
    std::string line;
    while (getline(ifs, line)) m_classes.push_back(line);

    // Give the configuration and weight files for the model
    std::string modelConfiguration = folderName + "yolov3.cfg";
    std::string modelWeights = folderName + "yolov3.weights";

    // Load the network
    m_net = cv::dnn::readNetFromDarknet(modelConfiguration, modelWeights);
    m_net.setPreferableBackend(cv::dnn::DNN_BACKEND_OPENCV);
    m_net.setPreferableTarget(cv::dnn::DNN_TARGET_CPU);
}

QVideoFrame MyVideoFilterRunnable::run(QVideoFrame *input,
                                       const QVideoSurfaceFormat &surfaceFormat,
                                       RunFlags flags) {
    Q_UNUSED(flags)

    if (!input->isValid()) {
        qWarning("Invalid input format");
        return *input;
    }

    m_frameRate = surfaceFormat.frameRate();

    m_frameCount++;

    m_Orientation = m_Filter ? m_Filter->property("orientation").toInt() : 0;

    m_Flip = surfaceFormat.scanLineDirection() == QVideoSurfaceFormat::BottomToTop;

    QImage image = QVideoFrameToQImage(input);
    if (image.isNull()) {
        return *input;
    }

    if (image.format() != QImage::Format_ARGB32) {
        image = image.convertToFormat(QImage::Format_ARGB32);
    }

    drawTrackingInfoYOLO(image);

    return QVideoFrame(image);
}

QImage MyVideoFilterRunnable::QVideoFrameToQImage(QVideoFrame* input) {
    switch (input->handleType()) {
    case QAbstractVideoBuffer::NoHandle:
        return QVideoFrameToQImage_using_Qt_internals(input);

    case QAbstractVideoBuffer::GLTextureHandle:
        return QVideoFrameToQImage_using_GLTextureHandle(input);

    case QAbstractVideoBuffer::XvShmImageHandle:
    case QAbstractVideoBuffer::CoreImageHandle:
    case QAbstractVideoBuffer::QPixmapHandle:
    case QAbstractVideoBuffer::EGLImageHandle:
    case QAbstractVideoBuffer::UserHandle:
        break;
    }

    return QImage();
}

QImage MyVideoFilterRunnable::QVideoFrameToQImage_using_Qt_internals(QVideoFrame* input) {
    return qt_imageFromVideoFrame(*input);
}

QImage MyVideoFilterRunnable::QVideoFrameToQImage_using_GLTextureHandle(QVideoFrame* input) {
    QImage image(input->width(), input->height(), QImage::Format_ARGB32);
    GLuint textureId = static_cast<GLuint>(input->handle().toInt());
    QOpenGLContext* ctx = QOpenGLContext::currentContext();
    QOpenGLFunctions* f = ctx->functions();
    GLuint fbo;
    f->glGenFramebuffers(1, &fbo);
    GLint prevFbo;
    f->glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prevFbo);
    f->glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    f->glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                              GL_TEXTURE_2D, textureId, 0);
    f->glReadPixels(0, 0, input->width(), input->height(),
                    GL_RGBA, GL_UNSIGNED_BYTE, image.bits());
    f->glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(prevFbo));
    return image;
}

void MyVideoFilterRunnable::drawTrackingInfo(QImage& image) {
    cv::Mat frame = QImageToCvMat(image);
    // Setup a rectangle to define your region of interest
    cv::Rect myROI(leftCrop, 0, 1280 - rightCrop, 720);

    // Crop the full image to that image contained by the rectangle myROI
    cv::Mat croppedFrame = frame(myROI);
    cv::Mat bgr;
    cv::cvtColor(croppedFrame, bgr, cv::COLOR_RGBA2BGR);
    dlib::array2d<dlib::bgr_pixel> dlibImage;
    dlib::assign_image(dlibImage, dlib::cv_image<dlib::bgr_pixel>(bgr));

    // frame from BGR to RGB ordering (dlib needs RGB ordering)
    cv::Mat rgb;
    cv::cvtColor(croppedFrame, rgb, cv::COLOR_RGBA2RGB);

    QPainter qPainter(&image);
    qPainter.setBrush(Qt::NoBrush);
    qPainter.setPen(Qt::red);
    qPainter.drawRect(leftCrop, 0, 1280 - rightCrop, 720);
    int xStart, xEnd, yStart, yEnd;
    if (m_frameCount % 20 == 0) {
        cv::Mat blob = cv::dnn::blobFromImage(bgr, 2.0 / 255,
                                              cv::Size(bgr.size[1], bgr.size[0]),
                                              cv::Scalar(127.5, 127.5, 127.5),
                                              false);
        m_net.setInput(blob);
        cv::Mat detections = m_net.forward();

        // Runs the forward pass to get output of the output layers
        std::vector<cv::Mat> outs;
        m_net.forward(outs, getOutputsNames(m_net));
        // Remove the bounding boxes with low confidence
        cv::Mat dec(detections.size[2], detections.size[3], CV_32F, detections.ptr<float>());
        float confidence;
        std::string label;
        int idx;

        for (int i = 0; i < dec.size[0]; i++) {
            confidence = dec.at<float>(i, 2);
            // filter out weak detections by requiring a minimum confidence
            if (confidence > confidenceThreshold) {
                // extract the index of the class label from the detections list
                idx = int(detections.at<float>(i, 1));
                if (idx < 0) continue;
                label = CLASSES[idx];

                if (label != "car" && label != "bus") continue;
                // compute the (x, y)-coordinates of the bounding box for the object
                xStart = int(detections.at<float>(i, 3) * image.width());
                yStart = int(detections.at<float>(i, 4) * image.height());
                xEnd = int(detections.at<float>(i, 5) * image.width());
                yEnd = int(detections.at<float>(i, 6) * image.height());

                // If the centre of one box is inside another box, we consider them the same
                // object. Will using IOU give a better result? This sound like a classic
                // problem, sure someone must have done a research about it
                bool tracked = false;
                dlib::rectangle pos;
                long xMean, yMean;
                for (auto tracker: m_trackers) {
                    pos = tracker.get_position();
                    xMean = (pos.left() + pos.right()) / 2;
                    yMean = (pos.top() + pos.bottom()) / 2;
                    if (xStart < xMean && xMean < xEnd && yStart < yMean && yMean < yEnd) {
                        tracked = true;
                        break;
                    }
                    // Hmm shall we check the other way as well??
                }
                if (!tracked) {
                    dlib::correlation_tracker tracker = dlib::correlation_tracker();
                    dlib::rectangle rect = dlib::rectangle(xStart, yStart, xEnd, yEnd);
                    tracker.start_track(dlibImage, rect);
                    m_trackers.push_back(tracker);
                    m_labels.push_back(std::to_string(m_trackers.size() - 1));
                    // put back the crop offset before we draw
                    qPainter.drawRect(xStart + leftCrop, yStart, xEnd - xStart, yEnd - yStart);
                    qPainter.drawText(xStart + leftCrop, yStart, QString::number(m_trackers.size() - 1));
                }
            }
        }
    }
    for (size_t i = 0; i < m_trackers.size(); i++) {
        m_trackers[i].update(dlibImage);
        dlib::rectangle pos = m_trackers[i].get_position();
        // unpack the position object
        xStart = int(pos.left());
        yStart = int(pos.top());
        xEnd = int(pos.right());
        yEnd = int(pos.bottom());
        // put back the crop offset before we draw
        qPainter.drawRect(xStart + leftCrop, yStart, xEnd - xStart, yEnd - yStart);
        qPainter.drawText(xStart + leftCrop, yStart, QString::fromStdString(m_labels[i]));
    }

    qPainter.end();
}

void MyVideoFilterRunnable::drawTrackingInfoYOLO(QImage& image) {
    cv::Mat frame = QImageToCvMat(image);
    // Setup a rectangle to define your region of interest
    cv::Rect myROI(leftCrop, 0, 1280 - leftCrop - rightCrop, 720);

    // Crop the full image to that image contained by the rectangle myROI
    cv::Mat croppedFrame = frame(myROI);
    cv::Mat bgr;    // for opencv
    cv::cvtColor(croppedFrame, bgr, cv::COLOR_RGBA2BGR);
    cv::Mat rgb;    // for dlib
    cv::cvtColor(croppedFrame, rgb, cv::COLOR_RGBA2RGB);
    dlib::array2d<dlib::bgr_pixel> dlibImage;
    dlib::assign_image(dlibImage, dlib::cv_image<dlib::bgr_pixel>(bgr));

    QPainter qPainter(&image);
    qPainter.setBrush(Qt::NoBrush);
    qPainter.setPen(Qt::red);
    dlib::rectangle pos;

    // track the speed of detected vehicle
    double distance;
    double dx, dy;
    if (m_frameCount % speedTrackPeriod == 0) {
        for (size_t i = 0; i < m_trackers.size(); i++) {
            pos = m_trackers[i].get_position();
            dx = ((pos.left() + pos.right()) / 2.0) - m_centroids[i].first;
            dy = ((pos.top() + pos.bottom()) / 2.0) - m_centroids[i].second;
            distance = dx * dx + dy * dy;

            if (distance < moveDistThreshold && m_startFrame[i] == 0) {
                m_startFrame[i] = m_frameCount;
            }

            m_centroids[i].first = (pos.left() + pos.right()) / 2;
            m_centroids[i].second = (pos.top() + pos.bottom()) / 2;
        }
    }

    if (m_frameCount % objectDetectionPeriod == 0) {
        cv::Mat blob;
        cv::dnn::blobFromImage(bgr, blob, 1/255.0, cvSize(inpWidth, inpHeight),
                               cv::Scalar(0,0,0), false, false);
        m_net.setInput(blob);

        // Runs the forward pass to get output of the output layers
        std::vector<cv::Mat> outs;
        m_net.forward(outs, getOutputsNames(m_net));
        // Remove the bounding boxes with low confidence
        std::vector<std::tuple<cv::Rect, int, float>> res = postprocess(bgr, outs);

        for (size_t i = 0; i < res.size(); i++) {
            cv::Rect box = std::get<0>(res[i]);
            std::string label = m_classes[size_t(std::get<1>(res[i]))];
            float confidence = std::get<2>(res[i]);
            // extract the index of the class label from the detections list
            if (label != "bus" && label != "truck") continue;

            // calculate IOU to check if the object is tracked or not
            bool tracked = false;
            for (auto tracker: m_trackers) {
                if (computeIOU(tracker.get_position(), box) > 0.1f) {
                    tracked = true;
                    break;
                }
            }
            if (!tracked) {
                dlib::correlation_tracker tracker = dlib::correlation_tracker();
                dlib::rectangle rect = dlib::rectangle(box.x, box.y,
                                                       box.x + box.width,
                                                       box.y + box.height);
                tracker.start_track(dlibImage, rect);
                m_trackers.push_back(tracker);
                m_labels.push_back(std::to_string(m_trackers.size() - 1));
                m_confidences.push_back(confidence);
                m_startFrame.push_back(0);
                m_centroids.push_back(std::make_pair(box.x + (box.width / 2), box.y + (box.height / 2)));
                // put back the crop offset before we draw
                qPainter.drawRect(box.x + leftCrop, box.y, box.width, box.height);
                qPainter.drawText(box.x + leftCrop, box.y,
                                  QString::number(m_trackers.size() - 1));
            }
        }
    }
    int xStart, xEnd, yStart, yEnd;

    QString infoStr;
    for (size_t i = 0; i < m_trackers.size(); i++) {
        m_trackers[i].update(dlibImage);
        pos = m_trackers[i].get_position();
        // unpack the position object
        xStart = int(pos.left());
        yStart = int(pos.top());
        xEnd = int(pos.right());
        yEnd = int(pos.bottom());

        // put back the crop offset before we draw
        qPainter.drawRect(xStart + leftCrop, yStart, xEnd - xStart, yEnd - yStart);
        if (m_startFrame[i] == 0) {
            infoStr = " moving";
        }
        else {
            infoStr = " time " + QString::number((m_frameCount - m_startFrame[i]) / m_frameRate) + "s";
        }
        qPainter.drawText(xStart + leftCrop, yStart, "ID: "
                          + QString::fromStdString(m_labels[i]) + infoStr);
    }



    qPainter.end();
}

cv::Mat MyVideoFilterRunnable::QImageToCvMat(const QImage &inImage, bool inCloneImageData) {
    switch (inImage.format()) {
     // 8-bit, 4 channel
     case QImage::Format_ARGB32:
     case QImage::Format_ARGB32_Premultiplied: {
        cv::Mat  mat( inImage.height(), inImage.width(),
                      CV_8UC4,
                      const_cast<uchar*>(inImage.bits()),
                      static_cast<size_t>(inImage.bytesPerLine())
                      );

        return (inCloneImageData ? mat.clone() : mat);
     }

     // 8-bit, 3 channel
     case QImage::Format_RGB32: {
        if ( !inCloneImageData ) {
           qWarning() << "ASM::QImageToCvMat() - Conversion requires cloning so we don't modify the original QImage data";
        }

        cv::Mat  mat( inImage.height(), inImage.width(),
                      CV_8UC4,
                      const_cast<uchar*>(inImage.bits()),
                      static_cast<size_t>(inImage.bytesPerLine())
                      );

        cv::Mat  matNoAlpha;

        cv::cvtColor( mat, matNoAlpha, cv::COLOR_BGRA2BGR );   // drop the all-white alpha channel

        return matNoAlpha;
     }

     // 8-bit, 3 channel
     case QImage::Format_RGB888: {
        if (!inCloneImageData) {
           qWarning() << "ASM::QImageToCvMat() - Conversion requires cloning so we don't modify the original QImage data";
        }

        QImage   swapped = inImage.rgbSwapped();

        return cv::Mat( swapped.height(), swapped.width(),
                        CV_8UC3,
                        const_cast<uchar*>(swapped.bits()),
                        static_cast<size_t>(swapped.bytesPerLine())
                        ).clone();
     }

     // 8-bit, 1 channel
     case QImage::Format_Indexed8: {
        cv::Mat  mat( inImage.height(), inImage.width(),
                      CV_8UC1,
                      const_cast<uchar*>(inImage.bits()),
                      static_cast<size_t>(inImage.bytesPerLine())
                      );

        return (inCloneImageData ? mat.clone() : mat);
     }

     default:
        qWarning() << "ASM::QImageToCvMat() - QImage format not handled in switch:" << inImage.format();
        break;
    }

    return cv::Mat();
}

// Remove the bounding boxes with low confidence using non-maxima suppression
std::vector<std::tuple<cv::Rect, int, float>> MyVideoFilterRunnable::postprocess(cv::Mat& frame,
                                                                                 const std::vector<cv::Mat>& outs) {
    std::vector<int> classIds;
    std::vector<float> confidences;
    std::vector<cv::Rect> boxes;

    for (size_t i = 0; i < outs.size(); ++i) {
        // Scan through all the bounding boxes output from the network and keep only the
        // ones with high confidence scores. Assign the box's class label as the class
        // with the highest score for the box.
        float* data = (float*)(outs[i].data);
        for (int j = 0; j < outs[i].rows; ++j, data += outs[i].cols) {
            cv::Mat scores = outs[i].row(j).colRange(5, outs[i].cols);
            cv::Point classIdPoint;
            double confidence;
            // Get the value and location of the maximum score
            minMaxLoc(scores, nullptr, &confidence, nullptr, &classIdPoint);
            if (confidence > double(confThreshold)) {
                int centerX = int(data[0] * frame.cols);
                int centerY = int(data[1] * frame.rows);
                int width = int(data[2] * frame.cols);
                int height = int(data[3] * frame.rows);
                int left = centerX - width / 2;
                int top = centerY - height / 2;

                classIds.push_back(classIdPoint.x);
                confidences.push_back(float(confidence));
                boxes.push_back(cv::Rect(left, top, width, height));
            }
        }
    }

    // Perform non maximum suppression to eliminate redundant overlapping boxes with
    // lower confidences
    std::vector<int> indices;
    cv::dnn::NMSBoxes(boxes, confidences, confThreshold, nmsThreshold, indices);
    std::vector<std::tuple<cv::Rect, int, float>> res;
    for (size_t i = 0; i < indices.size(); ++i) {
        size_t idx = size_t(indices[i]);
        res.push_back(std::make_tuple(boxes[idx], classIds[idx], confidences[idx]));
    }
    return res;
}

std::vector<std::string> MyVideoFilterRunnable::getOutputsNames(const cv::dnn::Net& net) {
    static std::vector<std::string> names;
    if (names.empty()) {
        //Get the indices of the output layers, i.e. the layers with unconnected outputs
        std::vector<int> outLayers = net.getUnconnectedOutLayers();

        //get the names of all the layers in the network
        std::vector<std::string> layersNames = net.getLayerNames();

        // Get the names of the output layers in names
        names.resize(outLayers.size());
        for (size_t i = 0; i < outLayers.size(); ++i)
        names[i] = layersNames[size_t(outLayers[i] - 1)];
    }
    return names;
}

float MyVideoFilterRunnable::computeIOU(dlib::rectangle boxA, cv::Rect boxB) {
    // If one rectangle is on left side of other
    if (boxA.left() > boxB.x + boxB.width || boxB.x > boxA.right()) return 0;

    // If one rectangle is above other
    if (boxA.top() > boxB.y + boxB.height || boxB.y > boxA.bottom()) return 0;

    // determine the (x, y)-coordinates of the intersection rectangle
    long xA = std::max(boxA.left(), long(boxB.x));
    long yA = std::max(boxA.top(), long(boxB.y));
    long xB = std::min(boxA.right(), long(boxB.x + boxB.width));
    long yB = std::min(boxA.bottom(), long(boxB.y + boxB.height));

    // compute the area of intersection rectangle
    long interArea = std::max(0l, xB - xA + 1) * std::max(0l, yB - yA + 1);

    // compute the area of both the prediction and ground-truth
    // rectangles
    long boxAArea = (boxA.right() - boxA.left() + 1) * (boxA.bottom() - boxA.top() + 1);
    long boxBArea = (boxB.width + 1) * (boxB.height + 1);

    // compute the intersection over union by taking the intersection
    // area and dividing it by the sum of prediction + ground-truth
    // areas - the interesection area
    float iou = interArea / float(boxAArea + boxBArea - interArea);

    // return the intersection over union value
    return iou;
}
