const path = require('path');
const fs = require('fs');

const configPath = path.join(__dirname, 'config.json');
const config = JSON.parse(fs.readFileSync(configPath, 'utf-8'));

module.exports = {
    email_user: config.email.user,
    email_pass: config.email.pass,
    redis_host: config.redis.host,
    redis_port: config.redis.port,
    redis_password: config.redis.password || '',
    code_prefix: 'verify_code_',
};
