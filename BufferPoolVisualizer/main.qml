import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15    // Para ListView

Window {
    id: root
    visible: true
    width: 800; height: 400
    title: "BufferPool Visualizer"

    Rectangle {  // DISCO
        id: diskArea; x:50; y:250; width:700; height:100
        color:"#e0e0e0"; border.color:"black"; border.width:1
        Text { anchors.centerIn: parent; text:"DISCO" }
    }

    Rectangle {  // MEMORIA
        id: memArea; x:50; y:50; width:700; height:100
        color:"#f0f0f0"; border.color:"black"; border.width:1
        Text { anchors.centerIn: parent; text:"MEMORIA" }
    }

    Component {  // página
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
         anchors.left: parent.left; anchors.right: parent.right
         anchors.bottom: parent.bottom
         height: 80
         model: ListModel { }
         delegate: Text {
             text: msg
             font.pixelSize: 12
             color: "black"
         }
         Rectangle { // fondo semitransparente
             anchors.fill: parent; color: "#ffffffcc"
         }
    }

    Column {
        anchors { left: parent.left; bottom: parent.bottom; margins: 20 }
        spacing: 5

        Text { text: ">> "; font.pointSize: 14 }

        TextField {                    // <<– TextField de QtQuick.Controls
            id: cmdInput
            placeholderText: "Escribe comando y Enter"
            width: 300
            onAccepted: {
                bridge.sendInput(text + "\n")
                text = ""
            }
        }
    }

    property int memCount: 0
    property int diskCount: 0

    Timer {
        interval: 200; running: true; repeat: true
        onTriggered: bridge.readStatus()
    }

    Connections {
        target: bridge
        onStatusUpdated: {
            // primero destruyo viejos rectángulos
            contentItem.children.forEach(function(c) {
                if (c.objectName.startsWith("frm")) c.destroy()
            })

            // cada línea: "Frame PageID Dirty PinCnt OpType LastAcc PinStat"
            for (let i = 0; i < rows.length; ++i) {
                let parts = rows[i].split(" ")
                let fid   = parseInt(parts[0])
                let pid   = parseInt(parts[1])
                let dirty = parts[2] === "1"
                let pinCnt= parseInt(parts[3])
                let op    = parts[4]
                let stat  = parseInt(parts[6])

                // crear rectángulo en memoria
                let pg = pageComp.createObject(root, {
                    objectName: "frm" + fid,
                    pageId: pid
                })
                // lo posicionamos secuencialmente:
                pg.x = memArea.x + fid*(pg.width + 10)
                pg.y = memArea.y + 35

                // color según estado:
                if (pinCnt > 0)      pg.color = "#ffaaaa"  // pineado
                else if (dirty)      pg.color = "#ffeeaa"  // sucio
                else                 pg.color = "lightblue"

                // texto adicional si quieres añadir op:
                pg.children[0].text = pid + op
            }
        }
    }
}