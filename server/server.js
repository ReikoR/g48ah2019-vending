const express = require('express');
const bodyParser = require('body-parser');
const app = express();
const port = 3000;

var http = require('http').Server(app);
var io = require('socket.io')(http);

app.use(bodyParser.text());

app.use(express.static('web'));

let state = '0,0,0,0';
let confirmCount = 0;

function getState() {
    return state + ',' + confirmCount;
}

io.on('connection', function(socket){
    console.log('a user connected');
});

app.get('/state', (req, res) => {
    res.send(getState());
});

app.get('/confirm', (req, res) => {
    console.log('Confirm');
    confirmCount++;
    res.sendStatus(200);

    io.sockets.emit('cart state', getState());
});

app.post('/state', (req, res) => {
    state = req.body;
    console.log(getState());
    res.sendStatus(200);

    io.sockets.emit('cart state', getState());
});

app.get('/finish', (req, res) => {
    console.log('Finish');
    confirmCount = 0;
    res.sendStatus(200);
});

http.listen(port, () => console.log(`Listening on port ${port}!`));