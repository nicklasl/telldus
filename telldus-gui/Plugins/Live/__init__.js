__setupPackage__( __extension__ );

com.telldus.live = function() {
    var socket = null;
    var menuId = 0;
	
	function init() {
		socket = new LiveSocket();
		socket.connected.connect(connected);
		socket.registered.connect(registered);
		socket.messageReceived.connect(messageReceived);
		socket.connectToServer();
   		com.telldus.core.deviceEvent.connect(deviceEvent);
	}
	
	function connected() {
   		if (com.telldus.systray) {
   		    com.telldus.systray.addSeparator();
			menuId = com.telldus.systray.addMenuItem( "Activate Telldus Live!" );
			com.telldus.systray.menuItem(menuId).triggered.connect(socket.activate);
		    com.telldus.systray.addSeparator();
		}
	}
	
	function deviceEvent(deviceId, method, data) {
	    msg = new LiveMessage("DeviceEvent");
	    msg.append(deviceId);
	    msg.append(method);
	    msg.append(data);
	    socket.sendMessage(msg);
	}
	
	function messageReceived(msg) {
	    if (msg.name() == "turnon") {
	        com.telldus.core.turnOn( msg.argument(0) );
	    } else if (msg.name() == "turnoff") {
	        com.telldus.core.turnOff( msg.argument(0) );
	    }
	    print("Received: " + msg.name());
	}
	
	function registered() {
	    if (menuId > 0) {
	        com.telldus.systray.removeMenuItem(menuId);
	        menuId = 0;
	    }
	    msg = new LiveMessage("DevicesReport");
	    var deviceList = com.telldus.core.deviceList.getList();
	    msg.append(deviceList.length);
		for( i in deviceList ) {
		    msg.append(deviceList[i].id);
		    msg.append(deviceList[i].name);
		}
		socket.sendMessage(msg);	
	}
	
	return { //Public functions
		init:init
	}
}();

__postInit__ = function() {
	application.allDoneLoading.connect( com.telldus.live.init );
}

