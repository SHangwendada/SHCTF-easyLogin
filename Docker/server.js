const http = require('http');
const url = require('url');
const PORT = 3000;


const FLAG = process.env.GZCTF_FLAG || 'flag{default_flag}';

const server = http.createServer((req, res) => {
    const queryObject = url.parse(req.url, true).query;
    const { username, password, passticket } = queryObject;

    console.log(`Received request with username: ${username}, password: ${password}, passticket: ${passticket}`);

    if (username !== 'SWDD') {
        res.writeHead(201, { 'Content-Type': 'text/plain' });
        res.end('Unauthorized: Invalid username.\n');
    } else if (password !== 'U1dERCBpcyB4aWFvbWVuZydzIERBRC4u') {
        res.writeHead(202, { 'Content-Type': 'text/plain' });
        res.end('Unauthorized: Invalid password.\n');
    } else {
        if (passticket !== '2161b139') {
            res.writeHead(203, { 'Content-Type': 'text/plain' });
            res.end('Invalid passticket.\n');
            return;
        }

        res.writeHead(200, { 'Content-Type': 'text/plain' });
        res.end(`${FLAG}\n`);
    }
});

server.listen(PORT, () => {
    console.log(`Server is running at http://localhost:${PORT}/`);
});
