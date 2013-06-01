var net = require('net');
var http = require('http');
var querystring = require('querystring');

var LISTEN_HOST = '0.0.0.0';
var LISTEN_PORT = 12345;

var PAYLOAD_LENGTH = 9;
var SPLIT_CHAR = 255;
var STUDENT_TIMEOUT = 10000;

var ACCESS_TOKEN = "helloworld123";
var REMOTE_SERVER = {
    host: 'api.shi6.com',
    path: '/ecard/notify',
    method: 'POST',
    headers: {
        'Content-Type': 'application/x-www-form-urlencoded',
        'Content-Length': 0
    },
};

var REQUEST_TIMEOUT = 3000;
var REQUEST_MIN_INTERVAL = 500;

var sending_timer = null;
function reset_sending_timer() {
    if (sending_timer) {
        set_sending_timer();
    } else {
        send_to_server();
    }
}
function set_sending_timer() {
    clearTimeout(sending_timer);
    sending_timer = setTimeout(reset_sending_timer, REQUEST_TIMEOUT);
}

var sending_queue;
var request_queue = [];
function send_to_server() {
    sending_queue = sending_queue.concat(request_queue);
    request_queue = [];
    if (sending_queue.length == 0)
        return;
    var post_data = querystring.stringify({
        token: ACCESS_TOKEN,
        data: json_encode(sending_queue),
    });
    REMOTE_SERVER.headers['Content-Length'] = post_data.length;
    var req = http.request(REMOTE_SERVER, function(res) {
        if (res.statusCode == 200) {
            console.log('Sent ' + sending_queue.length + ' packets');
            sending_queue = [];
        } else {
            console.log('HTTP error status: ' + res.statusCode);
        }
        // prevent too frequent requests
        setTimeout(reset_sending_timer, REQUEST_MIN_INTERVAL);
    });
    req.write(post_data);
    req.end();
}
function notify(no, direction) {
    request_queue.push([no, direction]);
    if (!sending_timer) {
        set_sending_timer();
        send_to_server();
    }
}
function notify_out(no) {
    notify(no, 0);
}
function notify_in(no) {
    notify(no, 1);
}

var timers = {};
var students = {};
function reset_student(no) {
    if (typeof students[no] !== 'undefined')
        students[no] = undefined;
}
function handle_packet(no, action) {
    var state_table = [
        [undefined, 1, 2],
        [undefined, 1, 3],
        [undefined, 4, 2],
        [undefined, 1, 3],
        [undefined, 4, 2],
    ];
    var timeout_actions = [
        null,
        reset_student,
        reset_student,
        notify_out,
        notify_in,
    ];
try {
    var curr_state = (typeof students[no] == 'undefined')
        ? state_table[0][action]
        : state_table[students[no]][action];
    students[no] = curr_state;
    if (typeof timers[no] !== 'undefined')
        clearTimeout(timers[no]);
    timers[no] = setTimeout(timeout_actions[curr_state], STUDENT_TIMEOUT);
} catch(e) {
    console.log('Exception in handle_packet:');
    console.log(e);
}
}

net.createServer(function(sock) {
    console.log('CONNECTED: ' + sock.remoteAddress + ':' + sock.remotePort);
    var action = sock.remoteAddress == '127.0.0.1' ? 1 : 2;
    var buf = '';
    sock.on('data', function(data) {
    try {
        buf += data;
        var packets = buf.split(chr(SPLIT_CHAR));
        for (i in packets) {
            if (packets[i].length == PAYLOAD_LENGTH)
                handle_packet(packets[i], action);
            // otherwise drop the packet
        }
        if (packets[packets.length-1].length != PAYLOAD_LENGTH) { // in case the last packet is not complete
            buf = packets[packets.length-1];
        } else {
            buf = '';
        }
    } catch(e) {
        console.log('Exception in data parsing:');
        console.log(e);
    }
    });
    sock.on('close', function(data) {
        console.log('CLOSED: ' + sock.remoteAddress + ':' + sock.remotePort);
    });
}).listen(LISTEN_PORT, LISTEN_HOST);

console.log('Server listening on ' + LISTEN_HOST + ':' + LISTEN_PORT);
