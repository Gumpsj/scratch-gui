const SerialPort = require("serialport");
const ListPort = require('serialport');
const Ready = require('@serialport/parser-ready')
const ByteLength = require('@serialport/parser-byte-length')
const Delimiter = require('@serialport/parser-delimiter')
const Readline = require('@serialport/parser-readline')

var com_port = 'COM1';

// find all serial ports and try to connect
function findCom(){
	ListPort.list(function (err, com_ports) {
	  com_ports.forEach(function(port) {
		  
		let sp = new SerialPort(port.comName, { 
			//autoOpen: false, 
			baudRate: 500000,
			parser: new SerialPort.parsers.Readline({ delimiter: '\r\n' }),
		});

		const raedyparser = sp.pipe(new Ready({ delimiter: 'ArduinoScratch' }))
		raedyparser.on('ready', function() {		
			console.log('Arduino found at ', sp.path);		
			com_port = sp.path;
		});
		
		sp.on("open", function () {
			console.log('Try serial port', port.comName);
			process.stdin.on('data', function (text) {
				sp.write(text, function(err, results) {});
				if (text === 'quit\n') {
				  process.exit();
				}
			});
			sp.on('data', function(data) {
				console.log('data received: ' + data);
			});
			setTimeout(function(){ 
				sp.close(function () {
					console.log('Port closed');
				});	 
			}, 2000);
		});

	  });
	});
}

function sleep(ms) {
  return new Promise(resolve => setTimeout(resolve, ms));
}

async function start() {
  console.log('Searching Arduino com port...');
  findCom();
  await sleep(5000);
  begin();
}


start();

function begin(){
	const CRC = require('crc-full').CRC;
	var crc = new CRC("CRC16_XMODEM", 16, 0x1021, 0x0000, 0x0000, false, false);

	var WebSocketServer = require('ws').Server; 					// include the webSocket library
	var SERVER_PORT = 8081;											// port number for the webSocket server
	var wss = new WebSocketServer({port: SERVER_PORT});				// the webSocket server
	var connections = new Array;									// list of connections to the server
	wss.binaryType = 'arraybuffer';
	var myPort = new SerialPort(com_port, { autoOpen: false, baudRate: 500000 });

	myPort.open(function () {		
		console.log('Arduino port ' + com_port + ' open.\nData rate: ' + myPort.baudRate);
		//const parser = myPort.pipe(new ByteLength({length: 16}))
		const parser = myPort.pipe(new Delimiter({ delimiter: '\t\n' }))
		myPort.on('close', showPortClose);						// called when the serial port closes
		myPort.on('error', showError);							// called when there's an error with the serial port
		parser.on('data', readSerialData);						// called when there's new data incoming		
	});	 
	

	// ------------------------ Serial event functions:
	function showPortClose() {
		console.log('Serial port closed.');
	}
	function showError(error) {
		console.log('Serial port error: ' + error);
	}

	function testSerial(){
		const msg = [0xF0, 0xF3, 0x03, 0x5E, 0x00, 0x00, 0x00, 0xF7 ]; 	
		var computed_crc = crc.compute(msg);
		msg.push(computed_crc >> 8);
		msg.push(computed_crc & 0xFF);
		
		value = value + 10;
		if(value > 0xFF)
			value = 0;
		console.log("Test msg: " + msg);
		myPort.write(msg);
	}

	// This is called when new data comes into the serial port:
	function readSerialData(data) {
		//console.log(data);
		if (connections.length > 0) {
			broadcast(data);
		}
	}

	async function sendToSerial(data) {
		if (data == '__ping__') {
			console.log('__pong__');
			broadcast('__pong__');
			return;
		}
		
		var serialMsg = new Uint8Array(10);
		serialMsg = data.split(',');
		//console.log("Sending to serial: " + serialMsg);
		myPort.write(serialMsg);
		await sleep(10);
		myPort.write(serialMsg);
	}

	// ------------------------ webSocket Server event functions
	wss.on('connection', handleConnection);

	function handleConnection(client) {
		console.log("New websocket Connection");					// you have a new client
		connections.push(client);						// add this client to the connections array
		client.on('message', sendToSerial);				// when a client sends a message,
		client.on('close', function() {					// when a client closes its connection
			console.log("connection closed");			// print it out
			var position = connections.indexOf(client);	// get the client's position in the array
			connections.splice(position, 1);			// and delete it from the array
		});
	}
	// This function broadcasts messages to all webSocket clients
	function broadcast(data) {
		for (c in connections) {						// iterate over the array of connections
			connections[c].send(JSON.stringify(data));	// send the data to each connection
		}
	}

}