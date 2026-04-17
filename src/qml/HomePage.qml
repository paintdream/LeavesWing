import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

Page {
    id: homePage

    ScrollView {
        anchors.fill: parent
        contentWidth: availableWidth

        ColumnLayout {
            width: parent.width
            spacing: 16

            // ── Welcome Banner ──────────────────────────────────────────
            Pane {
                Layout.fillWidth: true
                Layout.margins: 16
                Material.elevation: 2
                Material.background: "#EDE7F6"   // Light purple
                padding: 24

                ColumnLayout {
                    width: parent.width
                    spacing: 8

                    Label {
                        text: qsTr("Welcome to LeavesWing")
                        font.pixelSize: 22
                        font.weight: Font.Bold
                        color: "#4A148C"
                        Layout.fillWidth: true
                        wrapMode: Text.Wrap
                    }

                    Label {
                        text: qsTr("A cross-platform application powered by Qt 6.")
                        font.pixelSize: 14
                        color: "#6A1B9A"
                        Layout.fillWidth: true
                        wrapMode: Text.Wrap
                    }
                }
            }

            // ── Responsive Card Grid ────────────────────────────────────
            GridLayout {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                columns: homePage.width >= 600 ? 2 : 1
                rowSpacing: 12
                columnSpacing: 12

                Repeater {
                    model: [
                        { title: qsTr("Getting Started"), desc: qsTr("Learn the basics of the app.") },
                        { title: qsTr("Features"),        desc: qsTr("Discover what you can do.") },
                        { title: qsTr("Updates"),         desc: qsTr("See what's new in this version.") },
                        { title: qsTr("Support"),         desc: qsTr("Get help and contact us.") }
                    ]

                    delegate: Pane {
                        required property var modelData
                        Layout.fillWidth: true
                        Material.elevation: 1
                        padding: 16

                        ColumnLayout {
                            width: parent.width
                            spacing: 4

                            Label {
                                text: modelData.title
                                font.pixelSize: 16
                                font.weight: Font.Medium
                                color: "#512DA8"
                                Layout.fillWidth: true
                                wrapMode: Text.Wrap
                            }
                            Label {
                                text: modelData.desc
                                font.pixelSize: 13
                                color: "#757575"
                                Layout.fillWidth: true
                                wrapMode: Text.Wrap
                            }
                        }
                    }
                }
            }

            // Bottom spacer
            Item { Layout.fillHeight: true }
        }
    }
}
