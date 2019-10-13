import QtQuick 2.5
import QtQuick.Controls 2.1
import QtMultimedia 5.5
import MyVideoFilterLib 1.0

ApplicationWindow {
    id: filterItem
    visible: true
    width: 640
    height: 480
    title: qsTr("My Video Filter")

    VideoOutput {
      id: videoOutput

      anchors.fill: parent

      MediaPlayer {
          id: mediaPlayer
          volume: 0.5
          source: "file:///home/mihota/vision/vehicle-tracker/videographicsitem/TruckVideoShortVersion.mp4"
          autoPlay: true
      }

          autoOrientation: true
          source: mediaPlayer
          filters: [ videoFilter ]
      }

      MyVideoFilter {
          id: videoFilter

          orientation: videoOutput.orientation
      }
}
