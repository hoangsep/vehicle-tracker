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
    m_frameCount(0)
{
    std::string prototxt = "/home/mihota/vision/vehicle-tracker/videographicsitem/mobilenet_ssd/MobileNetSSD_deploy.prototxt";
    std::string model = "/home/mihota/vision/vehicle-tracker/videographicsitem/mobilenet_ssd/MobileNetSSD_deploy.caffemodel";
    m_net = cv::dnn::readNetFromCaffe(prototxt, model);
    std::cout << "Read net done" << std::endl;
}

QVideoFrame MyVideoFilterRunnable::run(QVideoFrame *input, const QVideoSurfaceFormat &surfaceFormat, RunFlags flags) {
    Q_UNUSED(flags)

    if (!input->isValid()) {
        qWarning("Invalid input format");
        return *input;
    }

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

//    drawRedGreenPixels(image);
    drawTrackingInfo(image);

    return QVideoFrame(image);
}

QImage MyVideoFilterRunnable::QVideoFrameToQImage(QVideoFrame* input)
{
    switch (input->handleType())
    {
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

QImage MyVideoFilterRunnable::QVideoFrameToQImage_using_Qt_internals(QVideoFrame* input)
{
    return qt_imageFromVideoFrame(*input);
}

QImage MyVideoFilterRunnable::QVideoFrameToQImage_using_GLTextureHandle(QVideoFrame* input)
{
    QImage image(input->width(), input->height(), QImage::Format_ARGB32);
    GLuint textureId = static_cast<GLuint>(input->handle().toInt());
    QOpenGLContext* ctx = QOpenGLContext::currentContext();
    QOpenGLFunctions* f = ctx->functions();
    GLuint fbo;
    f->glGenFramebuffers(1, &fbo);
    GLint prevFbo;
    f->glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prevFbo);
    f->glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    f->glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureId, 0);
    f->glReadPixels(0, 0, input->width(), input->height(), GL_RGBA, GL_UNSIGNED_BYTE, image.bits());
    f->glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(prevFbo));
    return image;
}

void MyVideoFilterRunnable::drawRedGreenPixels(QImage& image) {
    auto bits = image.bits();
    int bytesPerLine = image.bytesPerLine();
    auto bytesPerPixel = bytesPerLine / image.width();
    for (int y = 0; y < image.height() && y < 32; y++) {
        unsigned char* line = bits + y * bytesPerLine;
        auto leftPixel = line;
        auto rightPixel = line + bytesPerLine - bytesPerPixel;
        for (int x = 0; x < image.width() && x < 32; x++) {
            leftPixel[0] = 0;
            leftPixel[1] = 255;
            leftPixel[2] = 0;
            leftPixel += bytesPerPixel;
            rightPixel[0] = 0;
            rightPixel[1] = 0;
            rightPixel[2] = 255;
            rightPixel -= bytesPerPixel;
        }
    }
}

void MyVideoFilterRunnable::drawTrackingInfo(QImage& image) {
//    auto bits = image.bits();
//    int bytesPerLine = image.bytesPerLine();
//    auto bytesPerPixel = bytesPerLine / image.width();

//    cv::Mat frame(image.height(),image.width(),CV_8UC4, image.bits(), image.bytesPerLine());
    cv::Mat frame = QImageToCvMat(image);
    cv::Mat bgr;
    cv::cvtColor(frame, bgr, cv::COLOR_RGBA2BGR);
    dlib::array2d<dlib::bgr_pixel> dlibImage;
    dlib::assign_image(dlibImage, dlib::cv_image<dlib::bgr_pixel>(bgr));

    // frame from BGR to RGB ordering (dlib needs RGB ordering)
    cv::Mat rgb;
    cv::cvtColor(frame, rgb, cv::COLOR_RGBA2RGB);
    QPainter qPainter(&image);
    qPainter.setBrush(Qt::NoBrush);
    qPainter.setPen(Qt::red);
    int xStart, xEnd, yStart, yEnd;
    if (m_frameCount % 5 == 0) {
        //    cv::resize(bgr, bgr, cv::Size(), 0.75, 0.75);
        cv::Mat blob = cv::dnn::blobFromImage(bgr, 2.0 / 255, cv::Size(bgr.size[1], bgr.size[0]), cv::Scalar(127.5, 127.5, 127.5), false);
        m_net.setInput(blob);
        cv::Mat detections = m_net.forward();
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
//                std::cout << idx << ":" << label << ":" << confidence << std::endl;

                if (label != "car" && label != "bus") continue;
                // compute the (x, y)-coordinates of the bounding box for the object
                xStart = int(detections.at<float>(i, 3) * image.width());
                yStart = int(detections.at<float>(i, 4) * image.height());
                xEnd = int(detections.at<float>(i, 5) * image.width());
                yEnd = int(detections.at<float>(i, 6) * image.height());

    //            std::cout << xStart << ":" << xEnd << "-" << yStart << ":" << yEnd << std::endl;

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
                    qPainter.drawRect(xStart, yStart, xEnd - xStart, yEnd - yStart);
                    qPainter.drawText(xStart, yStart, QString::number(m_trackers.size() - 1));
                }

            }
        }
    }
    else {
        for (size_t i = 0; i < m_trackers.size(); i++) {
            m_trackers[i].update(dlibImage);
            dlib::rectangle pos = m_trackers[i].get_position();
            // unpack the position object
            xStart = int(pos.left());
            yStart = int(pos.top());
            xEnd = int(pos.right());
            yEnd = int(pos.bottom());
            qPainter.drawRect(xStart, yStart, xEnd - xStart, yEnd - yStart);
            qPainter.drawText(xStart, yStart, QString::fromStdString(m_labels[i]));
        }
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
