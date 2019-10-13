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
    m_Flip(0)
{
    std::string prototxt = "/home/mihota/vision/vehicle-tracker/videographicsitem/mobilenet_ssd/MobileNetSSD_deploy.prototxt";
    std::string model = "/home/mihota/vision/vehicle-tracker/videographicsitem/mobilenet_ssd/MobileNetSSD_deploy.caffemodel";
    net = cv::dnn::readNetFromCaffe(prototxt, model);
    std::cout << "Read net done" << std::endl;
}

QVideoFrame MyVideoFilterRunnable::run(QVideoFrame *input, const QVideoSurfaceFormat &surfaceFormat, RunFlags flags) {
    Q_UNUSED(flags)

    if (!input->isValid()) {
        qWarning("Invalid input format");
        return *input;
    }

    m_Orientation = m_Filter ? m_Filter->property("orientation").toInt() : 0;

    m_Flip = surfaceFormat.scanLineDirection() == QVideoSurfaceFormat::BottomToTop;

    QImage image = QVideoFrameToQImage(input);
    if (image.isNull()) {
        return *input;
    }

    if (image.format() != QImage::Format_ARGB32) {
        image = image.convertToFormat(QImage::Format_ARGB32);
    }

    drawRedGreenPixels(image);
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
    auto bits = image.bits();
    int bytesPerLine = image.bytesPerLine();
    auto bytesPerPixel = bytesPerLine / image.width();

//    cv::Mat frame(image.height(),image.width(),CV_8UC3, image.bits(), image.bytesPerLine());
    std::cout << image.format() << std::endl;
    cv::Mat frame;
//    QImageToCvMat(image, frame);
    frame = QImageToCvMat(image);
    cv::Mat bgr;
    cv::cvtColor(frame, bgr, cv::COLOR_RGBA2RGB);
//    frame = imutils.resize(frame, width=600)
    // frame from BGR to RGB ordering (dlib needs RGB ordering)
    cv::Mat rgb;
    cv::cvtColor(frame, rgb, cv::COLOR_RGBA2RGB);
//    cv::Mat blob = cv::dnn::blobFromImage(mat);
    cv::Mat blob;
    cv::dnn::blobFromImage(bgr, blob);
    net.setInput(blob);
    cv::Mat detections = net.forward("detection_out");
    std::cout << detections.size << std::endl;
//    std::cout << detections.at(0, 0) << std::endl;
}

//void MyVideoFilterRunnable::QImageToCvMat(const QImage& image, cv::OutputArray out) {

//    switch(image.format()) {
//        case QImage::Format_Invalid:
//        {
//            cv::Mat empty;
//            empty.copyTo(out);
//            break;
//        }
//        case QImage::Format_RGB32:
//        {
//            cv::Mat view(image.height(),image.width(),CV_8UC4,(void *)image.constBits(),image.bytesPerLine());
//            view.copyTo(out);
//            break;
//        }
//        case QImage::Format_RGB888:
//        {
//            cv::Mat view(image.height(),image.width(),CV_8UC3,(void *)image.constBits(),image.bytesPerLine());
//            cvtColor(view, out, cv::COLOR_RGB2BGR);
//            break;
//        }
//        default:
//        {
//            QImage conv = image.convertToFormat(QImage::Format_ARGB32);
//            cv::Mat view(conv.height(),conv.width(),CV_8UC4,(void *)conv.constBits(),conv.bytesPerLine());
//            view.copyTo(out);
//            break;
//        }
//    }
//}

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
