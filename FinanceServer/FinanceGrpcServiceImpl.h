#ifndef FINANCEGRPCSERVICEIMPL_H
#define FINANCEGRPCSERVICEIMPL_H

#include "grpcpp/grpcpp.h"
#include "message.grpc.pb.h"
#include "Global.h"

using grpc::Server;
using grpc::Status;
using grpc::ServerContext;
using grpc::ServerBuilder;

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

class FinanceGrpcServiceImpl final : public FinanceService::Service {
public:
    FinanceGrpcServiceImpl();

    Status CreatePayment(ServerContext* context, const CreatePaymentRequest* req, PaymentInfo* resp) override;
    Status ConfirmPayment(ServerContext* context, const PaymentInfo* req, PaymentInfo* resp) override;
    Status GetPaymentList(ServerContext* context, const PaymentListRequest* req, PaymentListResponse* resp) override;
    Status GetPaymentDetail(ServerContext* context, const PaymentInfo* req, PaymentInfo* resp) override;
    Status GenerateInvoice(ServerContext* context, const GenerateInvoiceRequest* req, InvoiceInfo* resp) override;
    Status GetInvoiceDetail(ServerContext* context, const InvoiceInfo* req, InvoiceInfo* resp) override;
    Status GetInvoiceList(ServerContext* context, const InvoiceListRequest* req, InvoiceListResponse* resp) override;
    Status GetStatsOverview(ServerContext* context, const CommonResponse* req, StatsOverviewResponse* resp) override;
    Status GetRevenueStats(ServerContext* context, const RevenueStatsRequest* req, RevenueStatsResponse* resp) override;
    Status GetVehicleStats(ServerContext* context, const CommonResponse* req, VehicleStatsResponse* resp) override;
};

#endif /* FINANCEGRPCSERVICEIMPL_H */
