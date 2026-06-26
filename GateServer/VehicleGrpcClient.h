#ifndef VEHICLEGRPCCLIENT_H
#define VEHICLEGRPCCLIENT_H

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

class VehicleGrpcClient : public Singleton<VehicleGrpcClient> {
    friend class Singleton<VehicleGrpcClient>;
public:
    // Vehicle CRUD
    VehicleListResponse getVehicleList(const VehicleListRequest& request);
    VehicleInfo getVehicleDetail(const VehicleDetailRequest& request);
    CommonResponse addVehicle(const AddVehicleRequest& request);
    CommonResponse updateVehicle(const UpdateVehicleRequest& request);
    CommonResponse deleteVehicle(const VehicleDetailRequest& request);

    // Rental orders
    OrderInfo createOrder(const CreateOrderRequest& request);
    OrderListResponse getOrderList(const OrderListRequest& request);
    OrderInfo getOrderDetail(const OrderDetailRequest& request);
    CommonResponse pickupVehicle(const PickupRequest& request);
    OrderInfo returnVehicle(const ReturnRequest& request);
    OrderInfo renewOrder(const RenewRequest& request);
    CommonResponse cancelOrder(const PickupRequest& request);

    // Maintenance
    CommonResponse createMaintenance(const CreateMaintenanceRequest& request);
    CommonResponse updateMaintenance(const UpdateMaintenanceRequest& request);
    MaintenanceListResponse getMaintenanceList(const MaintenanceListRequest& request);
    CommonResponse deleteMaintenance(const VehicleDetailRequest& request);

private:
    std::unique_ptr<VehicleConnectPool> rpc_pool_;
    VehicleGrpcClient();
};

#endif /* VEHICLEGRPCCLIENT_H */
