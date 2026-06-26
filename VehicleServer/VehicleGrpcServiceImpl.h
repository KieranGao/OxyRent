#ifndef VEHICLEGRPCSERVICEIMPL_H
#define VEHICLEGRPCSERVICEIMPL_H

#include "grpcpp/grpcpp.h"
#include "message.grpc.pb.h"
#include "Global.h"

using grpc::Server;
using grpc::Status;
using grpc::ServerContext;
using grpc::ServerBuilder;

using message::CommonResponse;
using message::VehicleInfo;
using message::VehicleListRequest;
using message::VehicleListResponse;
using message::VehicleDetailRequest;
using message::AddVehicleRequest;
using message::UpdateVehicleRequest;
using message::OrderInfo;
using message::CreateOrderRequest;
using message::OrderListRequest;
using message::OrderListResponse;
using message::OrderDetailRequest;
using message::PickupRequest;
using message::ReturnRequest;
using message::RenewRequest;
using message::MaintenanceInfo;
using message::CreateMaintenanceRequest;
using message::UpdateMaintenanceRequest;
using message::MaintenanceListRequest;
using message::MaintenanceListResponse;
using message::VehicleService;

class VehicleGrpcServiceImpl final : public VehicleService::Service {
public:
    VehicleGrpcServiceImpl();

    // 车辆管理
    Status GetVehicleList(ServerContext* context, const VehicleListRequest* req, VehicleListResponse* resp) override;
    Status GetVehicleDetail(ServerContext* context, const VehicleDetailRequest* req, VehicleInfo* resp) override;
    Status AddVehicle(ServerContext* context, const AddVehicleRequest* req, CommonResponse* resp) override;
    Status UpdateVehicle(ServerContext* context, const UpdateVehicleRequest* req, CommonResponse* resp) override;
    Status DeleteVehicle(ServerContext* context, const VehicleDetailRequest* req, CommonResponse* resp) override;

    // 租赁订单
    Status CreateOrder(ServerContext* context, const CreateOrderRequest* req, OrderInfo* resp) override;
    Status GetOrderList(ServerContext* context, const OrderListRequest* req, OrderListResponse* resp) override;
    Status GetOrderDetail(ServerContext* context, const OrderDetailRequest* req, OrderInfo* resp) override;
    Status PickupVehicle(ServerContext* context, const PickupRequest* req, CommonResponse* resp) override;
    Status ReturnVehicle(ServerContext* context, const ReturnRequest* req, OrderInfo* resp) override;
    Status RenewOrder(ServerContext* context, const RenewRequest* req, OrderInfo* resp) override;

    // 维保管理
    Status CreateMaintenance(ServerContext* context, const CreateMaintenanceRequest* req, CommonResponse* resp) override;
    Status UpdateMaintenance(ServerContext* context, const UpdateMaintenanceRequest* req, CommonResponse* resp) override;
    Status GetMaintenanceList(ServerContext* context, const MaintenanceListRequest* req, MaintenanceListResponse* resp) override;
};

#endif /* VEHICLEGRPCSERVICEIMPL_H */
