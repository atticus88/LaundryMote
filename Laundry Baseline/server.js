// This code will help monitor the current usage of the washing machine to get
// a better base line and improve alerting system
// data will be stored in a sqlite3 database
// graphed using google charts

// this code will be directly attached to the laundry mote 
// and read data from serail port
var express = require('express');
var sqlite3 = require('sqlite3');  // SCHEMA  ===   CREATE TABLE history (id INTEGER PRIMARY KEY, uptime VARCHAR(256), voltage INTEGER, alert INTEGER);
var db = new sqlite3.Database('voltage.db'); // Full Path to Database
var app = express();

app.set('views', __dirname + '/views');
app.use(express.static(__dirname + '/public'));
app.engine('html', require('ejs').renderFile);
app.use(express.bodyParser());
var server = app.listen(3030);
var io = require('socket.io').listen(server);

var serialport = require('serialport');
var serial = new serialport.SerialPort("/dev/tty.usbserial-A603BSK3", { baudrate : 115200, parser: serialport.parsers.readline("\n") });
 
serial.on("data", function (data) {
	var status = '';
	if (data.indexOf('{') != -1)  {
		var req = JSON.parse(data);
		var stmt = db.prepare("INSERT INTO history (uptime, voltage, alert) VALUES (?,?,?)"); // insert results into database
		stmt.run(req['uptime'], req['voltage'], req['alert']);
		stmt.finalize();
		io.sockets.emit('update', req);
	}
});


// need to pull page from directory easier to edit
app.get('/', function(req, res) {
	res.render('index.html');
});

// load historical data
app.get('/data', function(req, res) {
	db.all("SELECT * FROM history", function(err, row) {
		res.setHeader('content-type', 'json/application');
		res.send(row);
	});
});

// api for updating data on main dashboard page
// will not likley be used only for testing 
// curl -X POST -d '{"uptime" : "0", "alert" : 0, "voltage" : 0}' http://localhost:3030/update  --header "Content-Type: application/json"
app.post('/update', function(req, res) {
	var stmt = db.prepare("INSERT INTO history (uptime, voltage, alert) VALUES (?,?,?)");
	stmt.run(req.body.uptime, req.body.voltage, req.body.alert);
	stmt.finalize();
	io.sockets.emit('update', req.body);
	res.setHeader('content-type', 'json/application');
	res.send(req.body);
});



//start listening for connections socket.io
io.sockets.on('connection', function(socket) {

});
