const grpc = require('@grpc/grpc-js');
const protoLoader = require('@grpc/proto-loader');
const path = require('path');
const config = require('./config');
const emailModule = require('./email');
const constants = require('./const');

// 加载proto文件
const PROTO_PATH = path.join(__dirname, 'message.proto');
const packageDef = protoLoader.loadSync(PROTO_PATH, {
    keepCase: true,
    longs: String,
    enums: String,
    defaults: true,
    oneofs: true,
});
const proto = grpc.loadPackageDefinition(packageDef).message;

// Redis连接
const Redis = require('ioredis');
const redis = new Redis({
    host: config.redis_host,
    port: config.redis_port,
    password: config.redis_password || undefined,
    retryStrategy: (times) => Math.min(times * 200, 3000),
});

redis.on('connect', () => console.log('[邮件服务] Redis连接成功'));
redis.on('error', (err) => console.error('[邮件服务] Redis连接错误:', err.message));

// 生成6位随机验证码
function generateCode() {
    return Math.floor(100000 + Math.random() * 900000).toString();
}

// 实现SendMail RPC
async function sendMail(call, callback) {
    const { email, code } = call.request;
    console.log(`[邮件服务] 发送邮件到: ${email}`);

    try {
        const mailOptions = {
            from: `"OxyRent" <${config.email_user}>`,
            to: email,
            subject: 'OxyRent 验证码',
            text: `您的验证码是：${code}，5分钟内有效。如非本人操作，请忽略此邮件。`,
        };

        await emailModule.SendMail(mailOptions);
        console.log(`[邮件服务] 邮件发送成功: ${email}`);
        callback(null, { error: constants.Success });
    } catch (err) {
        console.error(`[邮件服务] 邮件发送失败: ${err.message}`);
        callback(null, { error: constants.Exception });
    }
}

// 实现SendVerifyCode RPC
async function sendVerifyCode(call, callback) {
    const { email } = call.request;
    console.log(`[邮件服务] 发送验证码到: ${email}`);

    try {
        // 生成验证码
        const code = generateCode();

        // 存储到Redis，5分钟过期
        const key = config.code_prefix + email;
        await redis.setex(key, 300, code);

        // 发送邮件
        const mailOptions = {
            from: `"OxyRent" <${config.email_user}>`,
            to: email,
            subject: 'OxyRent 验证码',
            text: `您的验证码是：${code}，5分钟内有效。如非本人操作，请忽略此邮件。`,
        };

        await emailModule.SendMail(mailOptions);
        console.log(`[邮件服务] 验证码发送成功: ${email}, 验证码: ${code}`);
        callback(null, { error: constants.Success, msg: '验证码已发送' });
    } catch (err) {
        console.error(`[邮件服务] 验证码发送失败: ${err.message}`);
        callback(null, { error: constants.Exception, msg: '验证码发送失败' });
    }
}

// 启动gRPC服务器
function main() {
    const server = new grpc.Server();
    server.addService(proto.MailerService.service, {
        SendMail: sendMail,
        SendVerifyCode: sendVerifyCode,
    });

    const address = `0.0.0.0:50056`;
    server.bindAsync(address, grpc.ServerCredentials.createInsecure(), (err, port) => {
        if (err) {
            console.error('[邮件服务] 启动失败:', err.message);
            process.exit(1);
        }
        console.log(`[邮件服务] gRPC服务器启动成功，监听端口: ${port}`);
    });
}

main();
