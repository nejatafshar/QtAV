import QtQuick 2.0
import "utils.js" as Utils

Page {
    title: qsTr("Misc")
    height: titleHeight + (1+glSet.visible*4)*Utils.kItemHeight + detail.contentHeight + 3*Utils.kSpacing

    Column {
        anchors.fill: content
        spacing: Utils.kSpacing
        Row {
            width: parent.width
            height: Utils.kItemHeight
            Button {
                text: qsTr("Preview")
                checkable: true
                checked: PlayerConfig.previewEnabled
                width: parent.width/3
                height: Utils.kItemHeight
                onCheckedChanged: PlayerConfig.previewEnabled = checked
            }
            Text {
                id: detail
                font.pixelSize: Utils.kFontSize
                color: "white"
                width: parent.width*2/3
                height: parent.height
                horizontalAlignment: Text.AlignHCenter
                text: qsTr("Press on the preview item to seek")
            }
        }
        Text {
            width: parent.width
            height: Utils.kItemHeight
            color: "white"
            font.pixelSize: Utils.kFontSize
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            visible: glSet.visible
            text: "OpenGL (" + qsTr("Restart to apply") + ")"
        }
        Row {
            id: glSet
            width: parent.width
            height: 3*Utils.kItemHeight
            spacing: Utils.kSpacing
            visible: Qt.platform.os === "windows"
            Menu {
                width: parent.width/3
                itemWidth: width
                model: ListModel {
                    id: glModel
                    ListElement { name: "Auto" }
                    ListElement { name: "Desktop" }
                    ListElement { name: "OpenGLES" }
                    ListElement { name: "Software" }
                }
                onClicked: {
                    var gl = glModel.get(index).name
                    PlayerConfig.openGLType = gl
                    if (gl === "OpenGLES") {
                        angle.sourceComponent = angleMenu
                    } else {
                        angle.sourceComponent = undefined
                    }
                }
                Component.onCompleted: {
                    for (var i = 0; i < glModel.count; ++i) {
                        console.log("gl: " + glModel.get(i).name)
                        if (PlayerConfig.openGLType === i) {
                            currentIndex = i
                            break
                        }
                    }
                }
            }
            Loader {
                id: angle
                width: parent.width/3
                height: parent.height
            }
            Component {
                id: angleMenu
                Menu {
                    anchors.fill: parent
                    itemWidth: width
                    model: ListModel {
                        id: angleModel
                        ListElement { name: "Auto"; detail: "D3D9 > D3D11" }
                        ListElement { name: "D3D9" }
                        ListElement { name: "D3D11" }
                        ListElement { name: "WARP" }
                    }
                    onClicked:  PlayerConfig.ANGLEPlatform = angleModel.get(index).name
                    Component.onCompleted: {
                        for (var i = 0; i < angleModel.count; ++i) {
                            console.log("d3d: " + angleModel.get(i).name)
                            if (PlayerConfig.ANGLEPlatform === angleModel.get(i).name) {
                                currentIndex = i
                                break
                            }
                        }
                    }
                }
            }
        }
    }
    Component.onCompleted: {
        if (PlayerConfig.openGLType === 2) {
            angle.sourceComponent = angleMenu
        }
    }
}
