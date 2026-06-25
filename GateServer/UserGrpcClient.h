#ifndef USERGRPCCLIENT_H
#define USERGRPCCLIENT_H

#include <grpcpp/grpcpp.h>
#include "message.grpc.pb.h"
#include "Global.h"
#include "Singleton.h"
#include "RPCConnectPool.h"
#include "ConfigManager.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

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
using message::UserService;

class UserGrpcClient : public Singleton<UserGrpcClient> {
    friend class Singleton<UserGrpcClient>;
public:
    CommonResponse userRegister(const UserRegisterRequest& request);
    UserLoginResponse userLogin(const UserLoginRequest& request);
    UserProfileResponse getUserProfile(const UserProfileRequest& request);
    CommonResponse updateProfile(const UpdateProfileRequest& request);
    UserListResponse getUserList(const UserListRequest& request);
    CommonResponse updateUserStatus(const UpdateUserStatusRequest& request);
    CommonResponse updateUserRole(const UpdateUserRoleRequest& request);
private:
    std::unique_ptr<UserConnectPool> rpc_pool_;
    UserGrpcClient();
};

#endif /* USERGRPCCLIENT_H */
