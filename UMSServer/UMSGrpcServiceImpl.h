#ifndef UMSGRPCSERVICEIMPL_H
#define UMSGRPCSERVICEIMPL_H

#include "grpcpp/grpcpp.h"
#include "message.grpc.pb.h"
#include "Global.h"

using grpc::Server;
using grpc::Status;
using grpc::ServerContext;
using grpc::ServerBuilder;

using message::UserRegisterRequest;
using message::UserLoginRequest;
using message::UserLoginResponse;
using message::UserProfileRequest;
using message::UserProfileResponse;
using message::UpdateProfileRequest;
using message::UserListRequest;
using message::UserListResponse;
using message::UpdateUserStatusRequest;
using message::UpdateUserRoleRequest;
using message::CommonResponse;
using message::GetBalanceRequest;
using message::GetBalanceResponse;
using message::TopupRequest;
using message::BalanceRecordListRequest;
using message::BalanceRecordListResponse;
using message::UserService;

class UMSGrpcServiceImpl final : public UserService::Service {
public:
    UMSGrpcServiceImpl();
    Status UserRegister(ServerContext* context, const UserRegisterRequest* req, CommonResponse* resp) override;
    Status UserLogin(ServerContext* context, const UserLoginRequest* req, UserLoginResponse* resp) override;
    Status GetUserProfile(ServerContext* context, const UserProfileRequest* req, UserProfileResponse* resp) override;
    Status UpdateProfile(ServerContext* context, const UpdateProfileRequest* req, CommonResponse* resp) override;
    Status GetUserList(ServerContext* context, const UserListRequest* req, UserListResponse* resp) override;
    Status UpdateUserStatus(ServerContext* context, const UpdateUserStatusRequest* req, CommonResponse* resp) override;
    Status UpdateUserRole(ServerContext* context, const UpdateUserRoleRequest* req, CommonResponse* resp) override;
    Status GetBalance(ServerContext* context, const GetBalanceRequest* req, GetBalanceResponse* resp) override;
    Status TopupBalance(ServerContext* context, const TopupRequest* req, CommonResponse* resp) override;
    Status GetBalanceRecords(ServerContext* context, const BalanceRecordListRequest* req, BalanceRecordListResponse* resp) override;
};

#endif /* UMSGRPCSERVICEIMPL_H */
