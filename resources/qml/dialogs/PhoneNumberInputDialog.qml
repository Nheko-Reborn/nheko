// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

import ".."
import QtQuick 2.12
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.3
import im.nheko 1.0

ApplicationWindow {
    id: inputDialog

    property alias echoMode: statusInput.echoMode
    property alias prompt: promptLabel.text

    signal accepted(string countryCode, string text)

    flags: Qt.Dialog
    height: fontMetrics.lineSpacing * 7
    modality: Qt.NonModal
    width: 350

    footer: DialogButtonBox {
        standardButtons: DialogButtonBox.Ok | DialogButtonBox.Cancel

        onAccepted: {
            inputDialog.accepted(numberPrefix.model.get(numberPrefix.currentIndex).i, statusInput.text);
            inputDialog.close();
        }
        onRejected: {
            inputDialog.close();
        }
    }

    GridLayout {
        anchors.fill: parent
        anchors.margins: Nheko.paddingMedium
        columnSpacing: Nheko.paddingMedium
        columns: 2
        rowSpacing: Nheko.paddingMedium

        Label {
            id: promptLabel

            Layout.columnSpan: 2
            color: palette.text
        }
        ComboBox {
            id: numberPrefix

            editable: false

            delegate: ItemDelegate {
                text: n + " (" + p + ")"
            }
            // taken from https://gitlab.com/whisperfish/whisperfish/-/blob/master/qml/js/countries.js

            //n=name,i=ISO,p=prefix -- see countries.js.md for source
            model: ListModel {
                ListElement {
                    i: "AF"
                    n: "Afghanistan"
                    p: "+93"
                }
                ListElement {
                    i: "AX"
                    n: "Åland Islands"
                    p: "+358 18"
                }
                ListElement {
                    i: "AL"
                    n: "Albania"
                    p: "+355"
                }
                ListElement {
                    i: "DZ"
                    n: "Algeria"
                    p: "+213"
                }
                ListElement {
                    i: "AS"
                    n: "American Samoa"
                    p: "+1 684"
                }
                ListElement {
                    i: "AD"
                    n: "Andorra"
                    p: "+376"
                }
                ListElement {
                    i: "AO"
                    n: "Angola"
                    p: "+244"
                }
                ListElement {
                    i: "AI"
                    n: "Anguilla"
                    p: "+1 264"
                }
                ListElement {
                    i: "AG"
                    n: "Antigua and Barbuda"
                    p: "+1 268"
                }
                ListElement {
                    i: "AR"
                    n: "Argentina"
                    p: "+54"
                }
                ListElement {
                    i: "AM"
                    n: "Armenia"
                    p: "+374"
                }
                ListElement {
                    i: "AW"
                    n: "Aruba"
                    p: "+297"
                }
                ListElement {
                    i: "SH"
                    n: "Ascension"
                    p: "+247"
                }
                ListElement {
                    i: "AU"
                    n: "Australia"
                    p: "+61"
                }
                ListElement {
                    i: "AQ"
                    n: "Australian Antarctic Territory"
                    p: "+672 1"
                }
                //ListElement{n:"Australian External Territories";i:"";p:"+672"} // NO ISO

                ListElement {
                    i: "AT"
                    n: "Austria"
                    p: "+43"
                }
                ListElement {
                    i: "AZ"
                    n: "Azerbaijan"
                    p: "+994"
                }
                ListElement {
                    i: "BS"
                    n: "Bahamas"
                    p: "+1 242"
                }
                ListElement {
                    i: "BH"
                    n: "Bahrain"
                    p: "+973"
                }
                ListElement {
                    i: "BD"
                    n: "Bangladesh"
                    p: "+880"
                }
                ListElement {
                    i: "BB"
                    n: "Barbados"
                    p: "+1 246"
                }
                ListElement {
                    i: "AG"
                    n: "Barbuda"
                    p: "+1 268"
                }
                ListElement {
                    i: "BY"
                    n: "Belarus"
                    p: "+375"
                }
                ListElement {
                    i: "BE"
                    n: "Belgium"
                    p: "+32"
                }
                ListElement {
                    i: "BZ"
                    n: "Belize"
                    p: "+501"
                }
                ListElement {
                    i: "BJ"
                    n: "Benin"
                    p: "+229"
                }
                ListElement {
                    i: "BM"
                    n: "Bermuda"
                    p: "+1 441"
                }
                ListElement {
                    i: "BT"
                    n: "Bhutan"
                    p: "+975"
                }
                ListElement {
                    i: "BO"
                    n: "Bolivia"
                    p: "+591"
                }
                ListElement {
                    i: "BQ"
                    n: "Bonaire"
                    p: "+599 7"
                }
                ListElement {
                    i: "BA"
                    n: "Bosnia and Herzegovina"
                    p: "+387"
                }
                ListElement {
                    i: "BW"
                    n: "Botswana"
                    p: "+267"
                }
                ListElement {
                    i: "BR"
                    n: "Brazil"
                    p: "+55"
                }
                ListElement {
                    i: "IO"
                    n: "British Indian Ocean Territory"
                    p: "+246"
                }
                ListElement {
                    i: "BN"
                    n: "Brunei Darussalam"
                    p: "+673"
                }
                ListElement {
                    i: "BG"
                    n: "Bulgaria"
                    p: "+359"
                }
                ListElement {
                    i: "BF"
                    n: "Burkina Faso"
                    p: "+226"
                }
                ListElement {
                    i: "BI"
                    n: "Burundi"
                    p: "+257"
                }
                ListElement {
                    i: "KH"
                    n: "Cambodia"
                    p: "+855"
                }
                ListElement {
                    i: "CM"
                    n: "Cameroon"
                    p: "+237"
                }
                ListElement {
                    i: "CA"
                    n: "Canada"
                    p: "+1"
                }
                ListElement {
                    i: "CV"
                    n: "Cape Verde"
                    p: "+238"
                }
                //ListElement{n:"Caribbean Netherlands";i:"";p:"+599 3"} // NO ISO

                //ListElement{n:"Caribbean Netherlands";i:"";p:"+599 4"} // NO ISO
                //ListElement{n:"Caribbean Netherlands";i:"";p:"+599 7"} // NO ISO
                ListElement {
                    i: "KY"
                    n: "Cayman Islands"
                    p: "+1 345"
                }
                ListElement {
                    i: "CF"
                    n: "Central African Republic"
                    p: "+236"
                }
                ListElement {
                    i: "TD"
                    n: "Chad"
                    p: "+235"
                }
                ListElement {
                    i: "NZ"
                    n: "Chatham Island (New Zealand)"
                    p: "+64"
                }
                ListElement {
                    i: "CL"
                    n: "Chile"
                    p: "+56"
                }
                ListElement {
                    i: "CN"
                    n: "China"
                    p: "+86"
                }
                ListElement {
                    i: "CX"
                    n: "Christmas Island"
                    p: "+61 89164"
                }
                ListElement {
                    i: "CC"
                    n: "Cocos (Keeling) Islands"
                    p: "+61 89162"
                }
                ListElement {
                    i: "CO"
                    n: "Colombia"
                    p: "+57"
                }
                ListElement {
                    i: "KM"
                    n: "Comoros"
                    p: "+269"
                }
                ListElement {
                    i: "CD"
                    n: "Congo (Democratic Republic of the)"
                    p: "+243"
                }
                ListElement {
                    i: "CG"
                    n: "Congo"
                    p: "+242"
                }
                ListElement {
                    i: "CK"
                    n: "Cook Islands"
                    p: "+682"
                }
                ListElement {
                    i: "CR"
                    n: "Costa Rica"
                    p: "+506"
                }
                ListElement {
                    i: "CI"
                    n: "Côte d'Ivoire"
                    p: "+225"
                }
                ListElement {
                    i: "HR"
                    n: "Croatia"
                    p: "+385"
                }
                ListElement {
                    i: "CU"
                    n: "Cuba"
                    p: "+53"
                }
                ListElement {
                    i: "CW"
                    n: "Curaçao"
                    p: "+599 9"
                }
                ListElement {
                    i: "CY"
                    n: "Cyprus"
                    p: "+357"
                }
                ListElement {
                    i: "CZ"
                    n: "Czech Republic"
                    p: "+420"
                }
                ListElement {
                    i: "DK"
                    n: "Denmark"
                    p: "+45"
                }
                //ListElement{n:"Diego Garcia";i:"";p:"+246"} // NO ISO, OCC. BY GB

                ListElement {
                    i: "DJ"
                    n: "Djibouti"
                    p: "+253"
                }
                ListElement {
                    i: "DM"
                    n: "Dominica"
                    p: "+1 767"
                }
                ListElement {
                    i: "DO"
                    n: "Dominican Republic"
                    p: "+1 809"
                }
                ListElement {
                    i: "DO"
                    n: "Dominican Republic"
                    p: "+1 829"
                }
                ListElement {
                    i: "DO"
                    n: "Dominican Republic"
                    p: "+1 849"
                }
                ListElement {
                    i: "CL"
                    n: "Easter Island"
                    p: "+56"
                }
                ListElement {
                    i: "EC"
                    n: "Ecuador"
                    p: "+593"
                }
                ListElement {
                    i: "EG"
                    n: "Egypt"
                    p: "+20"
                }
                ListElement {
                    i: "SV"
                    n: "El Salvador"
                    p: "+503"
                }
                ListElement {
                    i: "GQ"
                    n: "Equatorial Guinea"
                    p: "+240"
                }
                ListElement {
                    i: "ER"
                    n: "Eritrea"
                    p: "+291"
                }
                ListElement {
                    i: "EE"
                    n: "Estonia"
                    p: "+372"
                }
                ListElement {
                    i: "SZ"
                    n: "eSwatini"
                    p: "+268"
                }
                ListElement {
                    i: "ET"
                    n: "Ethiopia"
                    p: "+251"
                }
                ListElement {
                    i: "FK"
                    n: "Falkland Islands (Malvinas)"
                    p: "+500"
                }
                ListElement {
                    i: "FO"
                    n: "Faroe Islands"
                    p: "+298"
                }
                ListElement {
                    i: "FJ"
                    n: "Fiji"
                    p: "+679"
                }
                ListElement {
                    i: "FI"
                    n: "Finland"
                    p: "+358"
                }
                ListElement {
                    i: "FR"
                    n: "France"
                    p: "+33"
                }
                //ListElement{n:"French Antilles";i:"";p:"+596"} // NO ISO

                ListElement {
                    i: "GF"
                    n: "French Guiana"
                    p: "+594"
                }
                ListElement {
                    i: "PF"
                    n: "French Polynesia"
                    p: "+689"
                }
                ListElement {
                    i: "GA"
                    n: "Gabon"
                    p: "+241"
                }
                ListElement {
                    i: "GM"
                    n: "Gambia"
                    p: "+220"
                }
                ListElement {
                    i: "GE"
                    n: "Georgia"
                    p: "+995"
                }
                ListElement {
                    i: "DE"
                    n: "Germany"
                    p: "+49"
                }
                ListElement {
                    i: "GH"
                    n: "Ghana"
                    p: "+233"
                }
                ListElement {
                    i: "GI"
                    n: "Gibraltar"
                    p: "+350"
                }
                ListElement {
                    i: "GR"
                    n: "Greece"
                    p: "+30"
                }
                ListElement {
                    i: "GL"
                    n: "Greenland"
                    p: "+299"
                }
                ListElement {
                    i: "GD"
                    n: "Grenada"
                    p: "+1 473"
                }
                ListElement {
                    i: "GP"
                    n: "Guadeloupe"
                    p: "+590"
                }
                ListElement {
                    i: "GU"
                    n: "Guam"
                    p: "+1 671"
                }
                ListElement {
                    i: "GT"
                    n: "Guatemala"
                    p: "+502"
                }
                ListElement {
                    i: "GG"
                    n: "Guernsey"
                    p: "+44 1481"
                }
                ListElement {
                    i: "GG"
                    n: "Guernsey"
                    p: "+44 7781"
                }
                ListElement {
                    i: "GG"
                    n: "Guernsey"
                    p: "+44 7839"
                }
                ListElement {
                    i: "GG"
                    n: "Guernsey"
                    p: "+44 7911"
                }
                ListElement {
                    i: "GW"
                    n: "Guinea-Bissau"
                    p: "+245"
                }
                ListElement {
                    i: "GN"
                    n: "Guinea"
                    p: "+224"
                }
                ListElement {
                    i: "GY"
                    n: "Guyana"
                    p: "+592"
                }
                ListElement {
                    i: "HT"
                    n: "Haiti"
                    p: "+509"
                }
                ListElement {
                    i: "HN"
                    n: "Honduras"
                    p: "+504"
                }
                ListElement {
                    i: "HK"
                    n: "Hong Kong"
                    p: "+852"
                }
                ListElement {
                    i: "HU"
                    n: "Hungary"
                    p: "+36"
                }
                ListElement {
                    i: "IS"
                    n: "Iceland"
                    p: "+354"
                }
                ListElement {
                    i: "IN"
                    n: "India"
                    p: "+91"
                }
                ListElement {
                    i: "ID"
                    n: "Indonesia"
                    p: "+62"
                }
                ListElement {
                    i: "IR"
                    n: "Iran"
                    p: "+98"
                }
                ListElement {
                    i: "IQ"
                    n: "Iraq"
                    p: "+964"
                }
                ListElement {
                    i: "IE"
                    n: "Ireland"
                    p: "+353"
                }
                ListElement {
                    i: "IM"
                    n: "Isle of Man"
                    p: "+44 1624"
                }
                ListElement {
                    i: "IM"
                    n: "Isle of Man"
                    p: "+44 7524"
                }
                ListElement {
                    i: "IM"
                    n: "Isle of Man"
                    p: "+44 7624"
                }
                ListElement {
                    i: "IM"
                    n: "Isle of Man"
                    p: "+44 7924"
                }
                ListElement {
                    i: "IL"
                    n: "Israel"
                    p: "+972"
                }
                ListElement {
                    i: "IT"
                    n: "Italy"
                    p: "+39"
                }
                ListElement {
                    i: "JM"
                    n: "Jamaica"
                    p: "+1 876"
                }
                ListElement {
                    i: "SJ"
                    n: "Jan Mayen"
                    p: "+47 79"
                }
                ListElement {
                    i: "JP"
                    n: "Japan"
                    p: "+81"
                }
                ListElement {
                    i: "JE"
                    n: "Jersey"
                    p: "+44 1534"
                }
                ListElement {
                    i: "JO"
                    n: "Jordan"
                    p: "+962"
                }
                ListElement {
                    i: "KZ"
                    n: "Kazakhstan"
                    p: "+7 6"
                }
                ListElement {
                    i: "KZ"
                    n: "Kazakhstan"
                    p: "+7 7"
                }
                ListElement {
                    i: "KE"
                    n: "Kenya"
                    p: "+254"
                }
                ListElement {
                    i: "KI"
                    n: "Kiribati"
                    p: "+686"
                }
                ListElement {
                    i: "KP"
                    n: "Korea (North)"
                    p: "+850"
                }
                ListElement {
                    i: "KR"
                    n: "Korea (South)"
                    p: "+82"
                }
                // TEMP. CODE

                ListElement {
                    i: "XK"
                    n: "Kosovo"
                    p: "+383"
                }
                ListElement {
                    i: "KW"
                    n: "Kuwait"
                    p: "+965"
                }
                ListElement {
                    i: "KG"
                    n: "Kyrgyzstan"
                    p: "+996"
                }
                ListElement {
                    i: "LA"
                    n: "Laos"
                    p: "+856"
                }
                ListElement {
                    i: "LV"
                    n: "Latvia"
                    p: "+371"
                }
                ListElement {
                    i: "LB"
                    n: "Lebanon"
                    p: "+961"
                }
                ListElement {
                    i: "LS"
                    n: "Lesotho"
                    p: "+266"
                }
                ListElement {
                    i: "LR"
                    n: "Liberia"
                    p: "+231"
                }
                ListElement {
                    i: "LY"
                    n: "Libya"
                    p: "+218"
                }
                ListElement {
                    i: "LI"
                    n: "Liechtenstein"
                    p: "+423"
                }
                ListElement {
                    i: "LT"
                    n: "Lithuania"
                    p: "+370"
                }
                ListElement {
                    i: "LU"
                    n: "Luxembourg"
                    p: "+352"
                }
                ListElement {
                    i: "MO"
                    n: "Macau (Macao)"
                    p: "+853"
                }
                ListElement {
                    i: "MG"
                    n: "Madagascar"
                    p: "+261"
                }
                ListElement {
                    i: "MW"
                    n: "Malawi"
                    p: "+265"
                }
                ListElement {
                    i: "MY"
                    n: "Malaysia"
                    p: "+60"
                }
                ListElement {
                    i: "MV"
                    n: "Maldives"
                    p: "+960"
                }
                ListElement {
                    i: "ML"
                    n: "Mali"
                    p: "+223"
                }
                ListElement {
                    i: "MT"
                    n: "Malta"
                    p: "+356"
                }
                ListElement {
                    i: "MH"
                    n: "Marshall Islands"
                    p: "+692"
                }
                ListElement {
                    i: "MQ"
                    n: "Martinique"
                    p: "+596"
                }
                ListElement {
                    i: "MR"
                    n: "Mauritania"
                    p: "+222"
                }
                ListElement {
                    i: "MU"
                    n: "Mauritius"
                    p: "+230"
                }
                ListElement {
                    i: "YT"
                    n: "Mayotte"
                    p: "+262 269"
                }
                ListElement {
                    i: "YT"
                    n: "Mayotte"
                    p: "+262 639"
                }
                ListElement {
                    i: "MX"
                    n: "Mexico"
                    p: "+52"
                }
                ListElement {
                    i: "FM"
                    n: "Micronesia (Federated States of)"
                    p: "+691"
                }
                ListElement {
                    i: "US"
                    n: "Midway Island (USA)"
                    p: "+1 808"
                }
                ListElement {
                    i: "MD"
                    n: "Moldova"
                    p: "+373"
                }
                ListElement {
                    i: "MC"
                    n: "Monaco"
                    p: "+377"
                }
                ListElement {
                    i: "MN"
                    n: "Mongolia"
                    p: "+976"
                }
                ListElement {
                    i: "ME"
                    n: "Montenegro"
                    p: "+382"
                }
                ListElement {
                    i: "MS"
                    n: "Montserrat"
                    p: "+1 664"
                }
                ListElement {
                    i: "MA"
                    n: "Morocco"
                    p: "+212"
                }
                ListElement {
                    i: "MZ"
                    n: "Mozambique"
                    p: "+258"
                }
                ListElement {
                    i: "MM"
                    n: "Myanmar"
                    p: "+95"
                }
                // NO OWN ISO, DISPUTED

                ListElement {
                    i: "AZ"
                    n: "Nagorno-Karabakh"
                    p: "+374 47"
                }
                // NO OWN ISO, DISPUTED

                ListElement {
                    i: "AZ"
                    n: "Nagorno-Karabakh"
                    p: "+374 97"
                }
                ListElement {
                    i: "NA"
                    n: "Namibia"
                    p: "+264"
                }
                ListElement {
                    i: "NR"
                    n: "Nauru"
                    p: "+674"
                }
                ListElement {
                    i: "NP"
                    n: "Nepal"
                    p: "+977"
                }
                ListElement {
                    i: "NL"
                    n: "Netherlands"
                    p: "+31"
                }
                ListElement {
                    i: "KN"
                    n: "Nevis"
                    p: "+1 869"
                }
                ListElement {
                    i: "NC"
                    n: "New Caledonia"
                    p: "+687"
                }
                ListElement {
                    i: "NZ"
                    n: "New Zealand"
                    p: "+64"
                }
                ListElement {
                    i: "NI"
                    n: "Nicaragua"
                    p: "+505"
                }
                ListElement {
                    i: "NG"
                    n: "Nigeria"
                    p: "+234"
                }
                ListElement {
                    i: "NE"
                    n: "Niger"
                    p: "+227"
                }
                ListElement {
                    i: "NU"
                    n: "Niue"
                    p: "+683"
                }
                ListElement {
                    i: "NF"
                    n: "Norfolk Island"
                    p: "+672 3"
                }
                // OCC. BY TR

                ListElement {
                    i: "CY"
                    n: "Northern Cyprus"
                    p: "+90 392"
                }
                ListElement {
                    i: "GB"
                    n: "Northern Ireland"
                    p: "+44 28"
                }
                ListElement {
                    i: "MP"
                    n: "Northern Mariana Islands"
                    p: "+1 670"
                }
                ListElement {
                    i: "MK"
                    n: "North Macedonia"
                    p: "+389"
                }
                ListElement {
                    i: "NO"
                    n: "Norway"
                    p: "+47"
                }
                ListElement {
                    i: "OM"
                    n: "Oman"
                    p: "+968"
                }
                ListElement {
                    i: "PK"
                    n: "Pakistan"
                    p: "+92"
                }
                ListElement {
                    i: "PW"
                    n: "Palau"
                    p: "+680"
                }
                ListElement {
                    i: "PS"
                    n: "Palestine (State of)"
                    p: "+970"
                }
                ListElement {
                    i: "PA"
                    n: "Panama"
                    p: "+507"
                }
                ListElement {
                    i: "PG"
                    n: "Papua New Guinea"
                    p: "+675"
                }
                ListElement {
                    i: "PY"
                    n: "Paraguay"
                    p: "+595"
                }
                ListElement {
                    i: "PE"
                    n: "Peru"
                    p: "+51"
                }
                ListElement {
                    i: "PH"
                    n: "Philippines"
                    p: "+63"
                }
                ListElement {
                    i: "PN"
                    n: "Pitcairn Islands"
                    p: "+64"
                }
                ListElement {
                    i: "PL"
                    n: "Poland"
                    p: "+48"
                }
                ListElement {
                    i: "PT"
                    n: "Portugal"
                    p: "+351"
                }
                ListElement {
                    i: "PR"
                    n: "Puerto Rico"
                    p: "+1 787"
                }
                ListElement {
                    i: "PR"
                    n: "Puerto Rico"
                    p: "+1 939"
                }
                ListElement {
                    i: "QA"
                    n: "Qatar"
                    p: "+974"
                }
                ListElement {
                    i: "RE"
                    n: "Réunion"
                    p: "+262"
                }
                ListElement {
                    i: "RO"
                    n: "Romania"
                    p: "+40"
                }
                ListElement {
                    i: "RU"
                    n: "Russia"
                    p: "+7"
                }
                ListElement {
                    i: "RW"
                    n: "Rwanda"
                    p: "+250"
                }
                ListElement {
                    i: "BQ"
                    n: "Saba"
                    p: "+599 4"
                }
                ListElement {
                    i: "BL"
                    n: "Saint Barthélemy"
                    p: "+590"
                }
                ListElement {
                    i: "SH"
                    n: "Saint Helena"
                    p: "+290"
                }
                ListElement {
                    i: "KN"
                    n: "Saint Kitts and Nevis"
                    p: "+1 869"
                }
                ListElement {
                    i: "LC"
                    n: "Saint Lucia"
                    p: "+1 758"
                }
                ListElement {
                    i: "MF"
                    n: "Saint Martin (France)"
                    p: "+590"
                }
                ListElement {
                    i: "PM"
                    n: "Saint Pierre and Miquelon"
                    p: "+508"
                }
                ListElement {
                    i: "VC"
                    n: "Saint Vincent and the Grenadines"
                    p: "+1 784"
                }
                ListElement {
                    i: "WS"
                    n: "Samoa"
                    p: "+685"
                }
                ListElement {
                    i: "SM"
                    n: "San Marino"
                    p: "+378"
                }
                ListElement {
                    i: "ST"
                    n: "São Tomé and Príncipe"
                    p: "+239"
                }
                ListElement {
                    i: "SA"
                    n: "Saudi Arabia"
                    p: "+966"
                }
                ListElement {
                    i: "SN"
                    n: "Senegal"
                    p: "+221"
                }
                ListElement {
                    i: "RS"
                    n: "Serbia"
                    p: "+381"
                }
                ListElement {
                    i: "SC"
                    n: "Seychelles"
                    p: "+248"
                }
                ListElement {
                    i: "SL"
                    n: "Sierra Leone"
                    p: "+232"
                }
                ListElement {
                    i: "SG"
                    n: "Singapore"
                    p: "+65"
                }
                ListElement {
                    i: "BQ"
                    n: "Sint Eustatius"
                    p: "+599 3"
                }
                ListElement {
                    i: "SX"
                    n: "Sint Maarten (Netherlands)"
                    p: "+1 721"
                }
                ListElement {
                    i: "SK"
                    n: "Slovakia"
                    p: "+421"
                }
                ListElement {
                    i: "SI"
                    n: "Slovenia"
                    p: "+386"
                }
                ListElement {
                    i: "SB"
                    n: "Solomon Islands"
                    p: "+677"
                }
                ListElement {
                    i: "SO"
                    n: "Somalia"
                    p: "+252"
                }
                ListElement {
                    i: "ZA"
                    n: "South Africa"
                    p: "+27"
                }
                ListElement {
                    i: "GS"
                    n: "South Georgia and the South Sandwich Islands"
                    p: "+500"
                }
                // NO OWN ISO, DISPUTED

                ListElement {
                    i: "GE"
                    n: "South Ossetia"
                    p: "+995 34"
                }
                ListElement {
                    i: "SS"
                    n: "South Sudan"
                    p: "+211"
                }
                ListElement {
                    i: "ES"
                    n: "Spain"
                    p: "+34"
                }
                ListElement {
                    i: "LK"
                    n: "Sri Lanka"
                    p: "+94"
                }
                ListElement {
                    i: "SD"
                    n: "Sudan"
                    p: "+249"
                }
                ListElement {
                    i: "SR"
                    n: "Suriname"
                    p: "+597"
                }
                ListElement {
                    i: "SJ"
                    n: "Svalbard"
                    p: "+47 79"
                }
                ListElement {
                    i: "SE"
                    n: "Sweden"
                    p: "+46"
                }
                ListElement {
                    i: "CH"
                    n: "Switzerland"
                    p: "+41"
                }
                ListElement {
                    i: "SY"
                    n: "Syria"
                    p: "+963"
                }
                ListElement {
                    i: "SJ"
                    n: "Taiwan"
                    p: "+886"
                }
                ListElement {
                    i: "TJ"
                    n: "Tajikistan"
                    p: "+992"
                }
                ListElement {
                    i: "TZ"
                    n: "Tanzania"
                    p: "+255"
                }
                ListElement {
                    i: "TH"
                    n: "Thailand"
                    p: "+66"
                }
                ListElement {
                    i: "TL"
                    n: "Timor-Leste"
                    p: "+670"
                }
                ListElement {
                    i: "TG"
                    n: "Togo"
                    p: "+228"
                }
                ListElement {
                    i: "TK"
                    n: "Tokelau"
                    p: "+690"
                }
                ListElement {
                    i: "TO"
                    n: "Tonga"
                    p: "+676"
                }
                ListElement {
                    i: "MD"
                    n: "Transnistria"
                    p: "+373 2"
                }
                ListElement {
                    i: "MD"
                    n: "Transnistria"
                    p: "+373 5"
                }
                ListElement {
                    i: "TT"
                    n: "Trinidad and Tobago"
                    p: "+1 868"
                }
                ListElement {
                    i: "SH"
                    n: "Tristan da Cunha"
                    p: "+290 8"
                }
                ListElement {
                    i: "TN"
                    n: "Tunisia"
                    p: "+216"
                }
                ListElement {
                    i: "TR"
                    n: "Turkey"
                    p: "+90"
                }
                ListElement {
                    i: "TM"
                    n: "Turkmenistan"
                    p: "+993"
                }
                ListElement {
                    i: "TC"
                    n: "Turks and Caicos Islands"
                    p: "+1 649"
                }
                ListElement {
                    i: "TV"
                    n: "Tuvalu"
                    p: "+688"
                }
                ListElement {
                    i: "UG"
                    n: "Uganda"
                    p: "+256"
                }
                ListElement {
                    i: "UA"
                    n: "Ukraine"
                    p: "+380"
                }
                ListElement {
                    i: "AE"
                    n: "United Arab Emirates"
                    p: "+971"
                }
                ListElement {
                    i: "GB"
                    n: "United Kingdom"
                    p: "+44"
                }
                ListElement {
                    i: "US"
                    n: "United States"
                    p: "+1"
                }
                ListElement {
                    i: "UY"
                    n: "Uruguay"
                    p: "+598"
                }
                ListElement {
                    i: "UZ"
                    n: "Uzbekistan"
                    p: "+998"
                }
                ListElement {
                    i: "VU"
                    n: "Vanuatu"
                    p: "+678"
                }
                ListElement {
                    i: "VA"
                    n: "Vatican City State (Holy See)"
                    p: "+379"
                }
                ListElement {
                    i: "VA"
                    n: "Vatican City State (Holy See)"
                    p: "+39 06 698"
                }
                ListElement {
                    i: "VE"
                    n: "Venezuela"
                    p: "+58"
                }
                ListElement {
                    i: "VN"
                    n: "Vietnam"
                    p: "+84"
                }
                ListElement {
                    i: "VG"
                    n: "Virgin Islands (British)"
                    p: "+1 284"
                }
                ListElement {
                    i: "VI"
                    n: "Virgin Islands (US)"
                    p: "+1 340"
                }
                ListElement {
                    i: "US"
                    n: "Wake Island (USA)"
                    p: "+1 808"
                }
                ListElement {
                    i: "WF"
                    n: "Wallis and Futuna"
                    p: "+681"
                }
                ListElement {
                    i: "YE"
                    n: "Yemen"
                    p: "+967"
                }
                ListElement {
                    i: "ZM"
                    n: "Zambia"
                    p: "+260"
                }
                // NO OWN ISO, DISPUTED?

                ListElement {
                    i: "TZ"
                    n: "Zanzibar"
                    p: "+255 24"
                }
                ListElement {
                    i: "ZW"
                    n: "Zimbabwe"
                    p: "+263"
                }
            }
        }
        MatrixTextField {
            id: statusInput

            Layout.fillWidth: true
        }
    }
}
