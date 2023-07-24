var gateway = `ws://${window.location.hostname}/ws`;
var websocket;
window.addEventListener('load', onload);

var buttlist = {
    "butt1":0,
    "butt2":0,
    "butt3":0,
    "butt4":0,
}

function onload(event) {
    initWebSocket();
}

function getValues(){
    websocket.send("getValues");
}

function initWebSocket() {
    console.log('Trying to open a WebSocket connection…');
    websocket = new WebSocket(gateway);
    websocket.onopen = onOpen;
    websocket.onclose = onClose;
    websocket.onmessage = onMessage;
}

function onOpen(event) {
    console.log('Connection opened');
    getValues();
}

function onClose(event) {
    console.log('Connection closed');
    setTimeout(initWebSocket, 2000);
}

function dinamycSlider(val) {
    document.getElementById("sliderValue").innerHTML = val;
}

function updateSlider(element) {
    var sliderNumber = element.id.charAt(element.id.length-1);
    var sliderValue = document.getElementById(element.id).value;
    document.getElementById("sliderValue").innerHTML = sliderValue;
    // console.log(sliderValue);
    websocket.send(sliderNumber+"s"+sliderValue.toString());
}

function updateButton(element) {
    var buttonNumber = element.id.charAt(element.id.length-1);
    var buttonValue = 0;
    buttlist["butt"+buttonNumber] = !buttlist["butt"+buttonNumber];
    buttonValue = buttlist["butt"+buttonNumber];
    // document.getElementById("buttonValue").innerHTML = buttonValue;
    // console.log(buttonValue);
    websocket.send(buttonNumber+"b"+buttonValue.toString());
}

function onMessage(event) {
    console.log(event.data);
    var myObj = JSON.parse(event.data);
    var keys = Object.keys(myObj);
    // console.log(event.data);
    // document.getElementById("CurrentValue").value = myObj["CurrentValue"];
    // document.getElementById("slider1").value = myObj["sliderValue"];
    for (var i = 0; i < keys.length; i++){
        var key = keys[i];
        document.getElementById(key).innerHTML = myObj[key];
        if (key == "CurrentValue")break;        
        document.getElementById("slider"+ (i+1).toString()).value = myObj[key];
    }
}