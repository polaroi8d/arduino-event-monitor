// UI elements
let deviceNameLabel = document.getElementById('device-name');
let connectButton = document.getElementById('connect');
let disconnectButton = document.getElementById('disconnect');
let terminalContainer = document.getElementById('console');
let getConfigInfo = document.getElementById('getConfigInfo');

var tmpSenderBuffer = "";
var bufferArray = [];
var i;

let sendTypeTemperature = document.getElementById('sendTypeTemperature');
let sendTypeHumidity = document.getElementById('sendTypeHumidity');
let sendTypeDistance = document.getElementById('sendTypeDistance');
let sendTypeLight = document.getElementById('sendTypeLight');

let sendModeEvent = document.getElementById('sendModeEvent');
let sendModeSampling = document.getElementById('sendModeSampling');

let sendEdgeMode = document.getElementById('sendEdgeMode');
let sendtresholdModeMax = document.getElementById('sendtresholdModeMax');
let sendtresholdModeMin = document.getElementById('sendtresholdModeMin');
let sendEdgeSensorValue = document.getElementById('sendEdgeSensorValue');

let configType = document.getElementById('configType');
let configMode = document.getElementById('configMode');

let startProgram = document.getElementById('startProgram');
let stopProgram = document.getElementById('stopProgram');


startProgram.addEventListener('click', function(event) {
  console.log("Start program.");
  event.preventDefault();
  send('r');
});

stopProgram.addEventListener('click', function(event) {
  console.log("Stop program.");
  event.preventDefault();
  send('p');
});

getConfigInfo.addEventListener('click', function(event) {
  console.log("Get config info from MIC.");
  event.preventDefault();
  send('c');
});

sendModeEvent.addEventListener('click', function(event) {
  console.log("Send mode: event based.");
  event.preventDefault();
  send('e');
});

sendModeSampling.addEventListener('click', function(event) {
  console.log("Send mode: sampling.");
  event.preventDefault();
  send('s');
});

sendTypeTemperature.addEventListener('click', function(event) {
  console.log("Sensor set: temperature.");
  event.preventDefault();
  send('w');
});

sendTypeHumidity.addEventListener('click', function(event) {
  console.log("Sensor set: humidity.");
  event.preventDefault();
  send('h');
});

sendTypeDistance.addEventListener('click', function(event) {
  console.log("Sensor set: distance.");
  event.preventDefault();
  send('d');
});

sendTypeLight.addEventListener('click', function(event) {
  console.log("Sensor set: light.");
  event.preventDefault();
  send('l');
});

sendEdgeMode.addEventListener('click', function(event) {
  console.log("Treshold mode: EDGE");
  event.preventDefault();
  send('/');
});

sendtresholdModeMax.addEventListener('click', function(event) {
  console.log("Treshold mode: MAX");
  event.preventDefault();
  send('+');
});

sendtresholdModeMin.addEventListener('click', function(event) {
  console.log("Treshold mode: MAX");
  event.preventDefault();
  send('-');
});

sendEdgeSensorValue.addEventListener('click', function(event) {
  console.log("Get edge sensore value.");
  event.preventDefault();
  send('q');
});


// Helpers
let defaultDeviceName = 'Terminal';
let terminalAutoScrollingLimit = terminalContainer.offsetHeight / 2;
let isTerminalAutoScrolling = true;

function sendClock() {
  tmpSenderBuffer = 'C'
                    + document.getElementById("clockHours").value
                    + '/'
                    + document.getElementById("clockMinutes").value 
                    + ':';
  console.log("SENDER BUFFER [" + tmpSenderBuffer + "]");
  send(tmpSenderBuffer);

}

function sendEreaseFlag() {
  tmpSenderBuffer = 'E';
  console.log("SENDER BUFFER [" + tmpSenderBuffer + "]");
  send(tmpSenderBuffer);
}

function sendK() {
  tmpSenderBuffer = 'K';
  console.log("SENDER BUFFER [" + tmpSenderBuffer + "]");
  send(tmpSenderBuffer);
}

function sendTimeValue() {
  tmpSenderBuffer = 'T'+ (document.getElementById("timeValue").value*1000) +':';
  console.log("SENDER BUFFER [" + tmpSenderBuffer + "]");
  send(tmpSenderBuffer);
}

function statusWait(){
  document.getElementById("statusBar").className = "status-bar-wait";
  document.getElementById("statusBar").innerHTML = "Arduino is busy, please wait. <i class=\"fa fa-spinner fa-spin\" style=\"font-size:24px\"></i>";
}

function statusReady(){
  document.getElementById("statusBar").className = "status-bar-ready";
  document.getElementById("statusBar").innerHTML = "Arduino is ready for next command.";
}

var hiddenElement = document.createElement('a');

function downloadData() {
  console.log("Download the Data to PC.");

  hiddenElement.href = 'data:attachment/text,' + encodeURI(bufferArray);
  hiddenElement.target = '_blank';
  hiddenElement.download = 'data.csv';
  hiddenElement.click();
}

function scrollElement(element) {
  let scrollTop = element.scrollHeight - element.offsetHeight;

  if (scrollTop > 0) {
    element.scrollTop = scrollTop;
  }
}

function logToTerminal(message) {
  var date = new Date();
  let element =  date + ' $ ' + message + '\n';
  terminalContainer.insertAdjacentHTML('beforeend', element);

  if (isTerminalAutoScrolling) {
    scrollElement(terminalContainer);
  }
}

// Obtain configured Bluetooth Terminal instance
let terminal = new BluetoothTerminal();

// Override `receive` method to log incoming data to the terminal
terminal.receive = function(data) {
  if (data.includes("STATUS:READY")) {
    statusReady();
  } else if (data.includes("SENSOR:TRESHOLD")){
      var tresholdValue = data.split(':');
      document.getElementById("tresholdValue").value = tresholdValue[1];
  } else if (data.includes("DATA:TRANSFER")) {
      console.log("Data transfer is coming...");
  } else if (data.includes("D:")){
      var dataFromMic = data.split(':');
      bufferArray.push(dataFromMic[1]);
      console.log(data  + " was pushed the buffer");
  } else if (data.includes("DATA:END")) {
      console.log("End of data transfer...");
  } else {
    logToTerminal(data);
  }
};

// Override default log method to output messages to the terminal and console
terminal._log = function(...messages) {
  messages.forEach(message => {
    logToTerminal(message);
  });
};

// Implement own send function to log outcoming data to the terminal
function send(data) {
  statusWait();
  terminal.send(data);
}

// Bind event listeners to the UI elements
connectButton.addEventListener('click', function() {
  terminal.connect().
      then(() => {
        deviceNameLabel.textContent = terminal.getDeviceName() ?
            terminal.getDeviceName() : defaultDeviceName;
            statusReady();
            send("_");
      });
});

// disconnectButton.addEventListener('click', function() {
//   terminal.disconnect();
//   deviceNameLabel.textContent = 'Waiting for connection.';
// });


// Switch terminal auto scrolling if it scrolls out of bottom
terminalContainer.addEventListener('scroll', function() {
  let scrollTopOffset = terminalContainer.scrollHeight -
      terminalContainer.offsetHeight - terminalAutoScrollingLimit;

  isTerminalAutoScrolling = (scrollTopOffset < terminalContainer.scrollTop);
});
