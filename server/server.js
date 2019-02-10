const express = require('express');
const bodyParser = require('body-parser');
const app = express();
const port = 3000;

var http = require('http').Server(app);
var io = require('socket.io')(http);

app.use(bodyParser.text());

app.use(express.static('web'));

let state = '0,0,0,0,0';

io.on('connection', function(socket){
    console.log('a user connected');
});

app.get('/state', (req, res) => {
    res.send(state);
});

app.post('/state', (req, res) => {
    state = req.body;
    console.log(state);
    res.sendStatus(200);

    io.sockets.emit('cart state', state);
});

http.listen(port, () => console.log(`Listening on port ${port}!`));