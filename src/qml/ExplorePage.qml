import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

Page {
    id: explorePage

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // ── Search Bar ──────────────────────────────────────────────────
        Pane {
            Layout.fillWidth: true
            padding: 12
            Material.elevation: 1

            TextField {
                id: searchField
                anchors.fill: parent
                placeholderText: qsTr("Search...")
                Material.accent: "#7C4DFF"
                leftPadding: 12
            }
        }

        // ── Category Chips (horizontal scroll) ─────────────────────────
        ScrollView {
            Layout.fillWidth: true
            Layout.preferredHeight: 56
            ScrollBar.horizontal.policy: ScrollBar.AsNeeded
            ScrollBar.vertical.policy: ScrollBar.AlwaysOff
            contentWidth: chipRow.implicitWidth

            Row {
                id: chipRow
                spacing: 8
                leftPadding: 16
                rightPadding: 16
                topPadding: 8
                bottomPadding: 8

                Repeater {
                    model: [
                        qsTr("All"), qsTr("Popular"), qsTr("Recent"),
                        qsTr("Trending"), qsTr("Recommended")
                    ]

                    delegate: Button {
                        required property int index
                        required property string modelData
                        text: modelData
                        flat: index !== 0
                        Material.background: index === 0 ? "#7C4DFF" : "transparent"
                        Material.foreground: index === 0 ? "white" : "#7C4DFF"
                    }
                }
            }
        }

        // ── Content List ────────────────────────────────────────────────
        ListView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: 20

            delegate: ItemDelegate {
                required property int index
                width: ListView.view.width
                height: 72

                contentItem: RowLayout {
                    spacing: 16

                    Rectangle {
                        width: 48; height: 48
                        radius: 8
                        color: Qt.hsla(0.75, 0.5, 0.45 + index * 0.02, 1.0)

                        Label {
                            anchors.centerIn: parent
                            text: (index + 1).toString()
                            color: "white"
                            font.weight: Font.Bold
                        }
                    }

                    ColumnLayout {
                        spacing: 2
                        Layout.fillWidth: true

                        Label {
                            text: qsTr("Item %1").arg(index + 1)
                            font.pixelSize: 15
                            font.weight: Font.Medium
                            Layout.fillWidth: true
                            elide: Text.ElideRight
                        }
                        Label {
                            text: qsTr("Description for item %1").arg(index + 1)
                            font.pixelSize: 13
                            color: "#9E9E9E"
                            Layout.fillWidth: true
                            elide: Text.ElideRight
                        }
                    }
                }
            }
        }
    }
}
