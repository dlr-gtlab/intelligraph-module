import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Layouts 1.15
import QtCharts 2.15
import QtGraphicalEffects 1.15

Rectangle {
        id: rect
        implicitWidth: 400
        implicitHeight: 300
        color: "#152231"

        ChartView {
            title: "Line"
            anchors.fill: parent
            antialiasing: true
            theme: ChartView.ChartThemeBlueIcy

            LineSeries {
                name: "LineSeries"
                XYPoint { x: 0; y: 0 }
                XYPoint { x: 1.1; y: 2.1 }
                XYPoint { x: 1.9; y: 3.3 }
                XYPoint { x: 2.1; y: 2.1 }
                XYPoint { x: 2.9; y: 4.9 }
                XYPoint { x: 3.4; y: 3.0 }
                XYPoint { x: 4.1; y: 3.3 }
            }
        }
}
