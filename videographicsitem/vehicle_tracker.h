/**
 *This class should take the frame, process it, and then the video player will output it
 */

#ifndef VEHICLE_TRACKER_H
#define VEHICLE_TRACKER_H

#include <QMediaPlayer>
#include <QWidget>
#include <QAbstractVideoSurface>
#include <QVideoSurfaceFormat>
#include <iostream>

class VehicleTracker : public QAbstractVideoSurface {
    QList<QVideoFrame::PixelFormat> supportedPixelFormats(
            QAbstractVideoBuffer::HandleType handleType = QAbstractVideoBuffer::NoHandle) const
    {
        Q_UNUSED(handleType)

        // Return the formats you will support
        return QList<QVideoFrame::PixelFormat>() << QVideoFrame::Format_RGB565;
    }

    bool present(const QVideoFrame &frame)
    {
        Q_UNUSED(frame)
        // Handle the frame and do your processing
        std::cout << "frame received" << std::endl;
//        if (notMyFormat(frame.pixelFormat())) {
//            setError(IncorrectFormatError);
//            return false;
//        } else {

//            QVideoFrame frametodraw(frame);

//            if(!frametodraw.map(QAbstractVideoBuffer::ReadOnly))
//            {
//               setError(ResourceError);
//               return false;
//            }

//             //this is a shallow operation. it just refer the frame buffer
//             QImage image(
//                    frametodraw.bits(),
//                    frametodraw.width(),
//                    frametodraw.height(),
//                    frametodraw.bytesPerLine(),
//                    QImage::Format_RGB444);

//            mylabel->resize(image.size());

//            //QPixmap::fromImage create a new buffer for the pixmap
//            mylabel->setPixmap(QPixmap::fromImage(image));

//            //we can release the data
//            frametodraw.unmap();

//            mylabel->update();

            return true;
//        }
        return true;
    }
};

//class VehicleTracker : public QObject
//{
//    Q_OBJECT
//    Q_PROPERTY(QAbstractVideoSurface *videoSurface READ videoSurface WRITE setVideoSurface)

//public:
//    QAbstractVideoSurface* videoSurface() const { return m_surface; }

//    void setVideoSurface(QAbstractVideoSurface *surface)
//    {
//        if (m_surface != surface && m_surface && m_surface->isActive()) {
//            m_surface->stop();
//        }
//        m_surface = surface;
//        if (m_surface)
//            m_surface->start(m_format);
//    }

//    // ...

//public slots:
//    void onNewVideoContentReceived(const QVideoFrame &frame)
//    {
//        if (m_surface)
//            m_surface->present(frame);
//    }

//private:
//    QAbstractVideoSurface *m_surface;
//    QVideoSurfaceFormat m_format;
//};

#endif // VEHICLE_TRACKER_H
