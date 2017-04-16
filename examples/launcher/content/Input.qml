import QtQuick 2.2
import QtQuick.Controls 1.1
import QtQuick.Controls.Styles 1.1
import QtQuick.Layouts 1.1
import QtGraphicalEffects 1.0

TextField {
    Layout.fillWidth: true
    implicitHeight: 27
    style: TextFieldStyle {
        background: Item {
            /*RectangularGlow {
                anchors.fill: parent
                anchors.topMargin: 1
                anchors.leftMargin: -1
                anchors.bottomMargin: -anchors.topMargin
                anchors.rightMargin: -anchors.leftMargin
                glowRadius: 1
                spread: 0.0
                color: "black"
                cornerRadius: 1
            }*/
            Rectangle {
                anchors.fill: parent
                radius: 1
                color: "#131313"
                border.width: 1
                border.color: "#29292d"
            }
        }
        textColor:"white"
        placeholderTextColor: "#a7a7a7"
        font.pixelSize: 15
    }
}
