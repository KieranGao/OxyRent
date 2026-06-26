#ifndef MAILERGRPCCLIENT_H
#define MAILERGRPCCLIENT_H

#include <grpcpp/grpcpp.h>
#include "message.grpc.pb.h"
#include "Global.h"
#include "Singleton.h"
#include "RPCConnectPool.h"
#include "ConfigManager.h"
#include "Defer.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

using message::SendMailRequest;
using message::SendMailResponse;
using message::SendVerifyCodeRequest;
using message::SendVerifyCodeResponse;
using message::MailerService;

class MailerGrpcClient : public Singleton<MailerGrpcClient> {
    friend class Singleton<MailerGrpcClient>;
public:
    SendMailResponse sendMail(const SendMailRequest& request);
    SendVerifyCodeResponse sendVerifyCode(const SendVerifyCodeRequest& request);

private:
    std::unique_ptr<MailerConnectPool> rpc_pool_;
    MailerGrpcClient();
};

#endif /* MAILERGRPCCLIENT_H */
