import QtQuick 2.12
import QtQuick.Window 2.12
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.5
import QtQuick.Controls.Material 2.12
import QtQml 2.0

ApplicationWindow {
    id: root
    visible: true
    width: 640
    height: 480
    title: qsTr("test")

    ColumnLayout {
        id: rootLayout
        anchors.fill: parent
        spacing: 5
        anchors.margins: 5

        GroupBox {
            title: "Status: <font color=\"red\"><b>Offline</b></font>"
            Layout.fillWidth: true

            RowLayout {
                spacing: 5
                anchors.fill: parent

                Label {
                    text: "Address:"
                }

                TextField {
                    text: "192.168.10.1"
                }

                Label {
                    text: "Port:"
                }

                TextField {
                    text: "12345"
                }

                Button {
                    text: "Start"
                }
            }
        }
    }
}


