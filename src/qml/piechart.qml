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
            id: chart
            title: "Top-5 car brand shares in Finland"
            anchors.fill: parent
            legend.alignment: Qt.AlignBottom
            antialiasing: true
            theme: ChartView.ChartThemeBlueIcy

            PieSeries {
                id: pieSeries
                PieSlice { label: "Volkswagen"; value: 13.5 }
                PieSlice { label: "Toyota"; value: 10.9 }
                PieSlice { label: "Ford"; value: 8.6 }
                PieSlice { label: "Skoda"; value: 8.2 }
                PieSlice { label: "Volvo"; value: 6.8 }
            }
        }
}
