import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

ApplicationWindow {
    id: root

    width: 400
    height: 720
    visible: true
    title: qsTr("LeavesWing")

    // ── Purple Material Theme ───────────────────────────────────────────────
    Material.theme: Material.Light
    Material.primary: "#7C4DFF"       // Deep Purple A200
    Material.accent: "#7C4DFF"
    Material.foreground: "#212121"
    Material.background: "#FAFAFA"

    // ── Responsive helpers ──────────────────────────────────────────────────
    readonly property bool isLandscape: width > height
    readonly property bool isWide: width >= 600

    // ── Header ──────────────────────────────────────────────────────────────
    header: ToolBar {
        Material.background: "#7C4DFF"
        Material.foreground: "white"
        height: 48

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 16
            anchors.rightMargin: 16

            Label {
                text: tabBar.currentIndex === 0 ? qsTr("Home")
                    : tabBar.currentIndex === 1 ? qsTr("Explore")
                    : qsTr("Profile")
                font.pixelSize: 18
                font.weight: Font.Medium
                color: "white"
                Layout.fillWidth: true
            }
        }
    }

    // ── Page Stack ──────────────────────────────────────────────────────────
    StackLayout {
        id: pageStack
        anchors.fill: parent
        currentIndex: tabBar.currentIndex

        HomePage {}
        ExplorePage {}
        ProfilePage {}
    }

    // ── Bottom Tab Bar ──────────────────────────────────────────────────────
    footer: TabBar {
        id: tabBar
        Material.background: "white"
        Material.accent: "#7C4DFF"

        TabButton {
            text: qsTr("Home")
            icon.source: ""
            display: AbstractButton.TextUnderIcon
            Material.foreground: tabBar.currentIndex === 0 ? "#7C4DFF" : "#9E9E9E"

            contentItem: ColumnLayout {
                spacing: 2

                Label {
                    text: "\u2302"   // house
                    font.pixelSize: 22
                    horizontalAlignment: Text.AlignHCenter
                    Layout.fillWidth: true
                    color: tabBar.currentIndex === 0 ? "#7C4DFF" : "#9E9E9E"
                }
                Label {
                    text: qsTr("Home")
                    font.pixelSize: 11
                    horizontalAlignment: Text.AlignHCenter
                    Layout.fillWidth: true
                    color: tabBar.currentIndex === 0 ? "#7C4DFF" : "#9E9E9E"
                }
            }
        }

        TabButton {
            text: qsTr("Explore")
            display: AbstractButton.TextUnderIcon
            Material.foreground: tabBar.currentIndex === 1 ? "#7C4DFF" : "#9E9E9E"

            contentItem: ColumnLayout {
                spacing: 2

                Label {
                    text: "\u2603"   // compass-like
                    font.pixelSize: 22
                    horizontalAlignment: Text.AlignHCenter
                    Layout.fillWidth: true
                    color: tabBar.currentIndex === 1 ? "#7C4DFF" : "#9E9E9E"
                }
                Label {
                    text: qsTr("Explore")
                    font.pixelSize: 11
                    horizontalAlignment: Text.AlignHCenter
                    Layout.fillWidth: true
                    color: tabBar.currentIndex === 1 ? "#7C4DFF" : "#9E9E9E"
                }
            }
        }

        TabButton {
            text: qsTr("Profile")
            display: AbstractButton.TextUnderIcon
            Material.foreground: tabBar.currentIndex === 2 ? "#7C4DFF" : "#9E9E9E"

            contentItem: ColumnLayout {
                spacing: 2

                Label {
                    text: "\u263A"   // smiley / person
                    font.pixelSize: 22
                    horizontalAlignment: Text.AlignHCenter
                    Layout.fillWidth: true
                    color: tabBar.currentIndex === 2 ? "#7C4DFF" : "#9E9E9E"
                }
                Label {
                    text: qsTr("Profile")
                    font.pixelSize: 11
                    horizontalAlignment: Text.AlignHCenter
                    Layout.fillWidth: true
                    color: tabBar.currentIndex === 2 ? "#7C4DFF" : "#9E9E9E"
                }
            }
        }
    }
}
