import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15

Window {
    id: root
    visible: true; width:800; height:400
    title: "BufferPool Visualizer"

    Rectangle { id: diskArea; x:50; y:250; width:700; height:100
        color:"#e0e0e0"; border.color:"black"; border.width:1
        Text { anchors.centerIn: parent; text:"DISCO" }
    }

    Rectangle { id: memArea; x:50; y:50; width:700; height:100
        color:"#f0f0f0"; border.color:"black"; border.width:1
        Text { anchors.centerIn: parent; text:"MEMORIA" }
    }

    Component {
        id: pageComp
        Rectangle {
            property int pageId: -1
            width:50; height:30; radius:4
            color:"lightblue"; border.color:"blue"; border.width:1
            Text { anchors.centerIn: parent; text: pageId }
        }
    }

    // ─── Panel de Logs ───────────────────────────────
    ListView {
        id: logView
        anchors { left: parent.left; right: parent.right; bottom: parent.bottom }
        height: 80
        model: ListModel { }
        delegate: Text {
            text: msg
            font.pixelSize: 12
            color: "black"
        }
        Rectangle { anchors.fill: parent; color: "#ffffffcc" }
    }

    Column {
        anchors { left: parent.left; bottom: parent.bottom; margins:20 }
        spacing: 5
        Text { text: ">> "; font.pointSize: 14 }
        TextField {
            id: cmdInput; width:300
            placeholderText: "Escribe comando y Enter"
            onAccepted: {
                bridge.sendInput(text + "\n")
                logView.model.append({ "msg": ">> " + text })
                text = ""
            }
        }
    }

    Timer {
        interval: 200; running: true; repeat: true
        onTriggered: bridge.readStatus()
    }

    Connections {
        target: bridge
        onStatusUpdated: {
            // Limpio viejos frames
            contentItem.children.forEach(function(c) {
                if (c.objectName && c.objectName.startsWith("frm"))
                    c.destroy();
            })
            logView.model.append({ "msg": "#STATUS" })

            // Pinto nuevos
            for (var i = 0; i < rows.length; ++i) {
                var parts = rows[i].split(" ")
                var fid   = parseInt(parts[0])
                var pid   = parseInt(parts[1])
                var dirty = parts[2] === "1"
                var pinCnt= parseInt(parts[3])
                var op    = parts[4]
                var stat  = parseInt(parts[6])

                // Logging
                logView.model.append({ "msg": rows[i] })

                // crear rect
                var pg = pageComp.createObject(root, {
                    objectName: "frm" + fid,
                    pageId: pid
                })
                pg.x = memArea.x + fid*(pg.width + 10)
                pg.y = memArea.y + 35

                // color
                if (pinCnt > 0)      pg.color = "#ffaaaa"
                else if (dirty)      pg.color = "#ffeeaa"
                else                 pg.color = "lightblue"

                pg.children[0].text = pid + op
            }
        }
    }
}
