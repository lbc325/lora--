//webSocket初始化
function TestSockets() {
    //var socket = new WebSocket("ws://localhost:9000" + window.location.host + "/ws");
    var socket = new WebSocket("ws://localhost:9000/ws");
    //var socket = new WebSocket("ws://localhost:8888");

    socket.onclose = function(evt) {
        console.log("===========socket has been closed=======");
        socket.close();
    };
    socket.onerror = function(evt) {
        console.log("=====发生错误====="+evt.data);
    };
    return socket;
}

