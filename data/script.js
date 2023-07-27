var gateway = `ws://${window.location.hostname}/ws`;
var websocket;
window.addEventListener('load', onload);

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

function updateColor(element, state){
    if(state == true){            
        document.getElementById(element).classList.replace("butt", "butt_d");        
    }else{               
        document.getElementById(element).classList.replace("butt_d", "butt");
    }
  
}

function updateButton(element) {
    var buttonNumber = element.id.charAt(element.id.length-1);
    var buttonValue = 0;
    // buttlist["button"+buttonNumber] = buttlist["button"+buttonNumber] == !buttlist["button"+buttonNumber] ? buttlist["button"+buttonNumber] : !buttlist["button"+buttonNumber];
    // buttonValue = buttlist["button"+buttonNumber];
    websocket.send(buttonNumber+"b"+"a");

}

function onMessage(event) {
    console.log(event.data);
    var myObj = JSON.parse(event.data);
    var keys = Object.keys(myObj);
    for (var i = 0; i < keys.length; i++){
        var key = keys[i];       
        if (key == "buttonValue3") continue;
        document.getElementById(key).innerHTML = myObj[key];        
        if (key == "sliderValue") document.getElementById("slider1").value = myObj[key];        
    }    
    updateColor("button3",myObj["buttonValue3"]);    
}