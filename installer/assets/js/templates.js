const stringifyNice = (data) => JSON.stringify(data, null, 2)
const templates = Object.freeze({
    elements: {
        fileName: "elements.json",
        value: stringifyNice([
            {
                "name": "ssid",
                "type": "ACInput",
                "value": "",
                "label": "Wi-Fi SSID",
                "pattern": "",
                "placeholder": "",
                "style": "",
                "apply": "text"
            },
            {
                "name": "wifipassword",
                "type": "ACInput",
                "value": "",
                "label": "Wi-Fi Password",
                "pattern": "",
                "placeholder": "",
                "style": "",
                "apply": "text"
            },
            {
                "name": "websocket",
                "type": "ACInput",
                "value": "",
                "label": "WebSocket link from LNURLDevice extension in LNbits",
                "pattern": "",
                "placeholder": "",
                "style": "",
                "apply": "text"
            },
            {
                "name": "lnurl",
                "type": "ACInput",
                "value": "",
                "label": "Payment LNURL from LNURLDevice extension in LNbits",
                "pattern": "",
                "placeholder": "",
                "style": "",
                "apply": "text"
            },
            {
                "name": "servoclose",
                "type": "ACInput",
                "value": "0",
                "label": "Servo angle for tap closed",
                "pattern": "",
                "placeholder": "",
                "style": "",
                "apply": "text"
            },
            {
                "name": "servoopen",
                "type": "ACInput",
                "value": "180",
                "label": "Servo value for tap open",
                "pattern": "",
                "placeholder": "",
                "style": "",
                "apply": "text"
            },
            {
                "name": "tapduration",
                "type": "ACInput",
                "value": "6000",
                "label": "Duration tap is open (ms)",
                "pattern": "",
                "placeholder": "",
                "style": "",
                "apply": "text"
            }
        ])
    }
})

