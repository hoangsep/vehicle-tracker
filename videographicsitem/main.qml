import QtQuick 2.5
import QtQuick.Controls 2.1
import QtMultimedia 5.5
import MyVideoFilterLib 1.0
import QtQuick.Dialogs 1.3

ApplicationWindow {
    visible: true
    width: 640
    height: 480
    title: qsTr("My Vehicle Tracker")

    Button {
        id: uploadVideoBtn
        text: "Open Video"
        anchors {
            bottom: parent.bottom
            left: parent.left
            bottomMargin: 10
            leftMargin: 10
        }
        onClicked: importNewVideo.open()
    }

    /// import new video
    FileDialog {
        id: importNewVideo
        height: 400
        width: 600
        nameFilters: [ "Video files (*.mp4 *.avi *.mov *.wmv)"]
        title: qsTr("Import a video")
        onRejected: console.log("Cancel import of new video")
        onAccepted: console.log("fileUrl " + fileUrl)
    }

    VideoOutput {
        id: videoOutput

        anchors.fill: parent

        MediaPlayer {
            id: mediaPlayer
            volume: 0.5
            source: importNewVideo.fileUrl
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
