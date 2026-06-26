#ifndef FINANCEGRPCCLIENT_H
#define FINANCEGRPCCLIENT_H

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
using message::PaymentInfo;
using message::CreatePaymentRequest;
using message::PaymentListRequest;
using message::PaymentListResponse;
using message::InvoiceInfo;
using message::GenerateInvoiceRequest;
using message::InvoiceListRequest;
using message::InvoiceListResponse;
using message::StatsOverviewResponse;
using message::RevenueStatsRequest;
using message::RevenueStatsItem;
using message::RevenueStatsResponse;
using message::VehicleStatsResponse;
using message::VehicleStatsBrandItem;
using message::FinanceService;

class FinanceGrpcClient : public Singleton<FinanceGrpcClient> {
    friend class Singleton<FinanceGrpcClient>;
public:
    PaymentInfo createPayment(const CreatePaymentRequest& request);
    PaymentListResponse getPaymentList(const PaymentListRequest& request);
    PaymentInfo getPaymentDetail(const PaymentInfo& request);
    InvoiceInfo generateInvoice(const GenerateInvoiceRequest& request);
    InvoiceInfo getInvoiceDetail(const InvoiceInfo& request);
    InvoiceListResponse getInvoiceList(const InvoiceListRequest& request);
    StatsOverviewResponse getStatsOverview(const CommonResponse& request);
    RevenueStatsResponse getRevenueStats(const RevenueStatsRequest& request);
    VehicleStatsResponse getVehicleStats(const CommonResponse& request);

private:
    std::unique_ptr<FinanceConnectPool> rpc_pool_;
    FinanceGrpcClient();
};

#endif /* FINANCEGRPCCLIENT_H */
