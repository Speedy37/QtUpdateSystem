import QtQuick 2.2
import QtQuick.Controls 1.1
import QtQuick.Controls.Styles 1.1
import QtGraphicalEffects 1.0

Button {
    style: ButtonStyle {
        background: Item {
            RectangularGlow {
                anchors.fill: parent
                anchors.topMargin: 1
                anchors.leftMargin: -1
                anchors.bottomMargin: -anchors.topMargin
                anchors.rightMargin: -anchors.leftMargin
                glowRadius: 1
                spread: 0.0
                color: "black"
                cornerRadius: background.radius
            }
            Rectangle {
                id: background
                anchors.fill: parent
                radius: 3
                gradient: Gradient {
                    GradientStop { position: 0.0; color: "#565766" }
                    GradientStop { position: 0.01; color: control.hovered ? "#50525c" : "#393a43" }
                    GradientStop { position: 1.0; color: control.hovered ? "#3c3d45" : "#26272f" }
                }
            }
        }
        label: Text {
            text: control.text
            color: control.hovered ? "#dddddd" : "#a7a7a7"
            font.pixelSize: 15
            horizontalAlignment: Text.AlignHCenter
        }
    }
}
