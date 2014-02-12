LaundryMote
===========

####Base Line Utility Usage

Attach moteino to usb, and get the name of the device.  Edit the path to the device in server.js

```js
...
var serial = new serialport.SerialPort("/dev/tty.usbserial-A603BSK3", { baudrate : 115200, parser: serialport.parsers.readline("\n") });
...
```

Start the server

```
server@server$ node server.js
```

If the server errors out try starting it again somtimes the `LaundryMote: 915 Mhz....` will cause the server to stop.

Browse to http://localhost:3030/

