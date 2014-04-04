import QtQuick 2.0
import QtQuick.Window 2.0
import QtAV 1.3

Item {
        MediaPlayer { //or AVPlayer
            id: player
            source: "test.mov"
        }
        MouseArea {
            anchors.fill: parent
            onClicked: player.play()
        }
        VideoOutput {
            anchors.fill: parent
            source: player
        }
        Window {
            x: 0; y: 100; width: 600; height: 400; visible: true
            VideoOutput {
                anchors.fill: parent
                source: player
            }
        }
        Window {
            x: 600; y: 100; width: 400; height: 300; visible: true
            VideoOutput {
                anchors.fill: parent
                source: player
            }
        }

}
