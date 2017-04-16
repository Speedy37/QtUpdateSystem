import QtQuick 2.2
import QtQuick.Controls 1.1
import QtQuick.Controls.Styles 1.1
import QtQuick.Layouts 1.1
import QtQuick.Dialogs 1.1
import QtGraphicalEffects 1.0
import "content" as Launcher

Item {
    visible: true
    width: 804
    height: 105

    Rectangle {
        id: background
        width: parent.height
        height: parent.width
        anchors.centerIn: parent
        radius: 5
        border.width: 2
        border.color: "#080809"
        rotation: -90
        gradient: Gradient {

            GradientStop {
                position: 0
                color: "#2e294f"
            }
            GradientStop {
                position: 0.174
                color: "#5c529a"
            }

            GradientStop {
                position: 1
                color: "#1e1c31"
            }
        }
    }

    Item {
        id: header
        objectName: "caption"
        x: parent.x
        y: parent.y
        width: parent.width
        height: 55

        Rectangle
        {
            color : "#2e294f"
            radius: 5
            anchors.bottom: game_version.bottom
            anchors.bottomMargin: -4
            anchors.right: game_version.right
            anchors.rightMargin: -4
            anchors.top: game_version.top
            anchors.topMargin: -4
            anchors.left: parent.left
            anchors.leftMargin: 2
        }

        Text {
            id: game_version
            text: window.gameVersion
            font.family: "Arial"
            font.bold: true
            font.italic: true
            font.pointSize: 18
            font.letterSpacing:-2
            color: "#cbaff1"
            x: parent.x + 5
            y: parent.y + 14
        }

        Text {
            id: launcher_version
            anchors.top: header.top
            anchors.right: header.right
            anchors.rightMargin: 4
            anchors.topMargin: 0
            text: window.launcherVersion
            font.family: "Arial"
            font.pointSize: 10
            color: "#9b7fe1"
        }

        MouseArea {
            id: exit
            anchors.top: header.top
            anchors.right: header.right
            anchors.rightMargin: 13
            anchors.topMargin: 13
            width: 26
            height: 26
            onClicked: window.close()
            hoverEnabled : true
            Rectangle {
                visible: exit.containsMouse
                anchors.fill: parent
                radius: 4
                color: "black"
            }
            Text {
                text: qsTr("X")
                font.family: "Arial"
                font.bold: true
                font.pointSize: 18
                color: exit.containsMouse ? "#767676" : "#424242"
                anchors.fill: parent
                anchors.leftMargin: 5
                anchors.topMargin: -1
            }
        }

        MouseArea {
            id: minimize
            anchors.top: header.top
            anchors.right: exit.left
            anchors.rightMargin: 5
            anchors.topMargin: 13
            width: 26
            height: 26
            onClicked: window.showMinimized()
            hoverEnabled : true
            Rectangle {
                visible: minimize.containsMouse
                anchors.fill: parent
                radius: 4
                color: "black"
            }
            Text {
                text: qsTr("_")
                font.family: "Arial"
                font.bold: true
                font.pointSize: 18
                color: minimize.containsMouse ? "#767676" : "#424242"
                anchors.fill: parent
                anchors.leftMargin: 7
                anchors.topMargin: -6
            }
        }
    }

    Item {
        id: footer
        y:parent.y + parent.height - 35
        anchors.right: header.right
        anchors.left: header.left
        anchors.rightMargin: 25
        anchors.leftMargin: 25

        Launcher.Button {
            id: gameButton
            anchors.right: parent.right
            anchors.verticalCenter: parent.top
            width: 88
            height: 29
            text: window.gamelabel
            enabled: window.gamelabel.length > 0
            onClicked: window.onGameClicked()
        }

        Rectangle {
            id:progression
            anchors.left: parent.left
            anchors.verticalCenter: parent.top
            anchors.right: gameButton.left
            anchors.rightMargin: 10
            height: 29
            radius: 4
            gradient: Gradient {
                GradientStop {
                    position: 0
                    color: "#fb7d00"
                }

                GradientStop {
                    position: 1
                    color: "#3e1f01"
                }
            }

            Rectangle {
                id: progress_prepare
                width: parent.width * window.checkProgression
                anchors.left: parent.left
                anchors.leftMargin: 0
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 0
                anchors.top: parent.top
                anchors.topMargin: 0
                radius: 4
                gradient: Gradient {
                    GradientStop {
                        position: 0
                        color: "#484b52"
                    }

                    GradientStop {
                        position: 1
                        color: "#131315"
                    }
                }
            }

            Rectangle {
                id: progress_download
                width: parent.width * window.downloadProgression
                anchors.left: parent.left
                anchors.leftMargin: 0
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 0
                anchors.top: parent.top
                anchors.topMargin: 0
                radius: 4
                gradient: Gradient {
                    GradientStop {
                        position: 0
                        color: "#3a68e1"
                    }

                    GradientStop {
                        position: 1
                        color: "#142c6c"
                    }
                }
            }

            Rectangle {
                id: progress_apply
                width: parent.width * window.applyProgression
                anchors.left: parent.left
                anchors.leftMargin: 0
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 0
                anchors.top: parent.top
                anchors.topMargin: 0
                radius: 4
                gradient: Gradient {
                    GradientStop {
                        position: 0
                        color: "#1bf634"
                    }

                    GradientStop {
                        position: 1
                        color: "#0c8e1b"
                    }
                }
            }

            Text {
                id:progression_label
                color: "#ffffff"
                text: window.progressLabel
                style: Text.Normal
                textFormat: Text.PlainText
                renderType: Text.QtRendering
                anchors.fill: parent
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter
                font.pixelSize: 16
            }

            Glow {
                color: "#000000"
                anchors.fill: parent
                radius: 10
                spread: 0.4
                samples: 20
                source: progression_label
            }
        }

    }
}



