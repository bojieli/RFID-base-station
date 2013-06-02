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
function on_sending_timeout() {
    clearTimeout(sending_timer);
    sending_timer = null;
    send_to_server();
}
function reset_sending_timer() {
    clearTimeout(sending_timer);
    sending_timer = setTimeout(on_sending_timeout, REQUEST_TIMEOUT);
}

var sending_queue;
var request_queue = [];
function send_to_server() {
try {
    if (sending_timer)
        return;
    sending_queue = sending_queue.concat(request_queue);
    request_queue = [];
    if (sending_queue.length == 0)
        return; // if there is nothing to send, do not set the timer
    reset_sending_timer();
    var post_data = querystring.stringify({
        token: ACCESS_TOKEN,
        data: JSON.stringify(sending_queue),
    });
    REMOTE_SERVER.headers['Content-Length'] = post_data.length;
    var req = http.request(REMOTE_SERVER, function(res) {
        if (res.statusCode == 200) {
            console.log('Sent ' + sending_queue.length + ' packets');
            sending_queue = [];
        } else {
            console.log('HTTP error status: ' + res.statusCode);
        }
        // wait a minute to clear the timer to prevent too frequent requests
        setTimeout(on_sending_timeout, REQUEST_MIN_INTERVAL);
    });
    req.write(post_data);
    req.end();
} catch(e) {
    console.log('Exception in send_to_server:');
    console.log(e);
}
}
function notify(no, direction) {
    request_queue.push([no, direction]);
    send_to_server();
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
    debug_packet(no);
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
    var packet = [];
    sock.on('data', function(buf) {
    try {
        var i;
        for (i=0; i<buf.length; i++) {
            if (buf[i] == SPLIT_CHAR) {
                packet = [];
            } else {
                packet.push(buf[i]);
                if (packet.length == PAYLOAD_LENGTH)
                    handle_packet(buffer_stringify(packet), action);
            }
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

function buffer_stringify(arr) {
    var i, hex = '';
    for (i=0; i<arr.length; i++)
        hex += arr[i].toString(16);
    return hex;
}


var debug_packet = (function(){
    var counter = 0;
    var errors = 0;
    var last = 255;
    return function(no) {
        ++counter;
        var curr = parseInt(no.substr(-2, 2), 16);
        if (curr != 0 && last + 1 != curr)
            ++errors;
        last = curr;
        if (counter % 1000 == 0)
            console.log('total ' + counter + ', errors ' + errors + ', rate ' + errors/counter);
    }
})();
