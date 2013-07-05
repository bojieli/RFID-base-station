var http = require('http');

function http_server(request, response) {
    response.except = function(e) {
        var message = (typeof e.message === "string") ? e.message : e.toString();

        if (typeof e.stack === "string")
            console.log(e.stack);
        else
            console.log(message);

        this.writeHeader(400);
        this.write(message);
        this.end();
    }
    response.returnOK = function() {
        console.log("200 OK");
        this.writeHeader(200);
        this.write("OK");
        this.end();
    }
    response.return200 = function(str) {
        console.log("200 (length " + str.length + ")");
        this.writeHeader(200);
        this.write(str);
        this.end();
    }
    try {
        var pathname = url.parse(request.url).pathname;
        if (request.method == "POST") {
            var data = "";
            request.on("data", function(chunk) {
                data += chunk;
            });
            request.on("end", function() {
                console.log(pathname);
                console.log(data);
                route(pathname, request.headers, data, response);
            });
            request.on("error", function(e) {
                response.except(e);
            });
        }
        else throw "All requests must be POSTed";
    } catch(e) {
        response.except(e);
    }
}

var proxy = http.createServer(http_server).listen(54322, 'localhost');

