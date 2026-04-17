import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

Page {
    id: profilePage

    ScrollView {
        anchors.fill: parent
        contentWidth: availableWidth

        ColumnLayout {
            width: parent.width
            spacing: 0

            // ── Avatar Section ──────────────────────────────────────────
            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: 200

                Rectangle {
                    anchors.fill: parent
                    gradient: Gradient {
                        GradientStop { position: 0.0; color: "#7C4DFF" }
                        GradientStop { position: 1.0; color: "#B388FF" }
                    }
                }

                ColumnLayout {
                    anchors.centerIn: parent
                    spacing: 12

                    Rectangle {
                        Layout.alignment: Qt.AlignHCenter
                        width: 80; height: 80
                        radius: 40
                        color: "white"

                        Label {
                            anchors.centerIn: parent
                            text: "\u263A"
                            font.pixelSize: 40
                            color: "#7C4DFF"
                        }
                    }

                    Label {
                        Layout.alignment: Qt.AlignHCenter
                        text: qsTr("User Name")
                        font.pixelSize: 18
                        font.weight: Font.Medium
                        color: "white"
                    }

                    Label {
                        Layout.alignment: Qt.AlignHCenter
                        text: qsTr("user@example.com")
                        font.pixelSize: 13
                        color: "#E1BEE7"
                    }
                }
            }

            // ── Menu Items ──────────────────────────────────────────────
            Repeater {
                model: [
                    { label: qsTr("Account Settings"), icon: "\u2699" },
                    { label: qsTr("Notifications"),    icon: "\u266A" },
                    { label: qsTr("Privacy"),          icon: "\u26BF" },
                    { label: qsTr("Help & Feedback"),  icon: "\u2753" },
                    { label: qsTr("About"),            icon: "\u2139" }
                ]

                delegate: ItemDelegate {
                    required property var modelData
                    required property int index
                    width: profilePage.width
                    height: 56

                    contentItem: RowLayout {
                        spacing: 16

                        Label {
                            text: modelData.icon
                            font.pixelSize: 20
                            color: "#7C4DFF"
                            Layout.preferredWidth: 32
                            horizontalAlignment: Text.AlignHCenter
                        }

                        Label {
                            text: modelData.label
                            font.pixelSize: 15
                            Layout.fillWidth: true
                        }

                        Label {
                            text: "\u203A"
                            font.pixelSize: 20
                            color: "#BDBDBD"
                        }
                    }

                    Rectangle {
                        anchors.bottom: parent.bottom
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.leftMargin: 64
                        height: 1
                        color: "#EEEEEE"
                        visible: index < 4
                    }
                }
            }

            // ── Sign Out Button ─────────────────────────────────────────
            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: 80

                Button {
                    anchors.centerIn: parent
                    text: qsTr("Sign Out")
                    flat: true
                    Material.foreground: "#F44336"
                    font.pixelSize: 15
                }
            }

            Item { Layout.fillHeight: true }
        }
    }
}
