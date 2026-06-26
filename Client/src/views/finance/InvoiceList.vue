<template>
  <div class="page-container">
    <div class="glass-card">
      <div class="glass-card-header">
        <h3>账单管理</h3>
        <div class="header-controls">
          <el-button type="primary" @click="showGenerateDialog">
            <el-icon><Plus /></el-icon> 生成账单
          </el-button>
        </div>
      </div>
      <div class="glass-card-body">
        <el-table :data="invoices" v-loading="loading" style="width: 100%" empty-text="暂无账单">
          <el-table-column prop="id" label="ID" min-width="60" />
          <el-table-column prop="invoice_no" label="账单号" min-width="160" />
          <el-table-column prop="order_no" label="订单号" min-width="160" />
          <el-table-column prop="username" label="客户" min-width="100" />
          <el-table-column prop="total_amount" label="金额" min-width="100" align="right">
            <template #default="{ row }">
              <span class="text-accent">¥{{ (row.total_amount || 0).toFixed(2) }}</span>
            </template>
          </el-table-column>
          <el-table-column prop="generated_at" label="生成时间" min-width="160" />
          <el-table-column label="操作" min-width="100">
            <template #default="{ row }">
              <el-button link type="primary" @click="viewDetail(row)">详情</el-button>
            </template>
          </el-table-column>
        </el-table>

        <div class="pagination-wrapper" v-if="total > pageSize">
          <el-pagination
            background
            layout="prev, pager, next"
            :total="total"
            :page-size="pageSize"
            :current-page="page"
            @current-change="handlePageChange"
          />
        </div>
      </div>
    </div>

    <!-- 生成账单对话框 -->
    <el-dialog v-model="dialogVisible" title="生成账单" width="400px">
      <el-form ref="formRef" :model="form" :rules="rules" label-position="top">
        <el-form-item label="选择订单" prop="order_id">
          <el-select
            v-model="form.order_id"
            filterable
            placeholder="请选择已完成的订单"
            style="width: 100%"
            :loading="ordersLoading"
          >
            <el-option
              v-for="order in rentalOrders"
              :key="order.id"
              :label="`${order.order_no} - ${order.username} - ${order.plate_number}`"
              :value="order.id"
            />
          </el-select>
        </el-form-item>
      </el-form>
      <template #footer>
        <el-button @click="dialogVisible = false">取消</el-button>
        <el-button type="primary" :loading="generating" @click="handleGenerate">生成</el-button>
      </template>
    </el-dialog>

    <!-- 账单详情对话框 -->
    <el-dialog v-model="detailVisible" title="账单详情" width="550px">
      <div v-if="invoiceDetail" class="invoice-detail">
        <div class="detail-row"><span class="label">账单号</span><span>{{ invoiceDetail.invoice_no }}</span></div>
        <div class="detail-row"><span class="label">订单号</span><span>{{ invoiceDetail.order_no }}</span></div>
        <div class="detail-row"><span class="label">客户</span><span>{{ invoiceDetail.username }}</span></div>
        <div class="detail-row"><span class="label">金额</span><span class="text-accent" style="font-weight: 600">¥{{ (invoiceDetail.total_amount || 0).toFixed(2) }}</span></div>
        <div class="detail-row"><span class="label">生成时间</span><span>{{ invoiceDetail.generated_at }}</span></div>
        <div class="detail-row" v-if="invoiceDetail.items">
          <span class="label">明细</span>
          <div class="items-list">
            <div v-for="(item, idx) in parsedItems" :key="idx" class="item-entry">
              <span>{{ item.desc || item.description || '费用' }}</span>
              <span class="text-accent">¥{{ (item.amount || 0).toFixed(2) }}</span>
            </div>
          </div>
        </div>
      </div>
    </el-dialog>
  </div>
</template>

<script setup>
import { ref, reactive, computed, onMounted } from 'vue'
import { getInvoiceList, getInvoiceDetail, generateInvoice } from '@/api/finance'
import { getRentalList } from '@/api/rental'
import { ElMessage } from 'element-plus'
import { Plus } from '@element-plus/icons-vue'

const loading = ref(false)
const invoices = ref([])
const total = ref(0)
const page = ref(1)
const pageSize = ref(20)
const dialogVisible = ref(false)
const detailVisible = ref(false)
const generating = ref(false)
const formRef = ref(null)
const invoiceDetail = ref(null)
const rentalOrders = ref([])
const ordersLoading = ref(false)

const form = reactive({ order_id: undefined })
const rules = {
  order_id: [{ required: true, message: '请选择订单', trigger: 'change' }],
}

const parsedItems = computed(() => {
  if (!invoiceDetail.value || !invoiceDetail.value.items) return []
  try {
    return JSON.parse(invoiceDetail.value.items)
  } catch {
    return []
  }
})

async function loadInvoices() {
  loading.value = true
  try {
    const res = await getInvoiceList({ page: page.value, page_size: pageSize.value })
    if (res.error === 0) {
      invoices.value = res.invoices || []
      total.value = res.total || 0
    }
  } catch {
    ElMessage.error('加载账单列表失败')
  } finally {
    loading.value = false
  }
}

function handlePageChange(p) {
  page.value = p
  loadInvoices()
}

async function loadCompletedOrders() {
  ordersLoading.value = true
  try {
    const res = await getRentalList({ status: 'completed', page: 1, page_size: 200 })
    if (res.error === 0) {
      rentalOrders.value = res.orders || []
    }
  } catch {
    ElMessage.error('加载订单列表失败')
  } finally {
    ordersLoading.value = false
  }
}

function showGenerateDialog() {
  form.order_id = undefined
  dialogVisible.value = true
  loadCompletedOrders()
}

async function handleGenerate() {
  const valid = await formRef.value.validate().catch(() => false)
  if (!valid) return
  generating.value = true
  try {
    const res = await generateInvoice({ order_id: form.order_id })
    if (res.error === 0) {
      ElMessage.success('账单已生成')
      dialogVisible.value = false
      loadInvoices()
    } else {
      ElMessage.error(res.message || '生成账单失败')
    }
  } catch {
    ElMessage.error('生成账单失败')
  } finally {
    generating.value = false
  }
}

async function viewDetail(row) {
  try {
    const res = await getInvoiceDetail({ id: row.id })
    if (res.error === 0) {
      invoiceDetail.value = res
      detailVisible.value = true
    }
  } catch {
    ElMessage.error('加载账单详情失败')
  }
}

onMounted(() => {
  loadInvoices()
})
</script>

<style scoped>
.header-controls {
  display: flex;
  gap: 10px;
  align-items: center;
}

.pagination-wrapper {
  display: flex;
  justify-content: center;
  padding: 20px 0 8px;
}

.invoice-detail .detail-row {
  display: flex;
  justify-content: space-between;
  padding: 12px 0;
  border-bottom: 1px solid var(--border);
  font-size: 14px;
}

.invoice-detail .detail-row:last-child {
  border-bottom: none;
}

.invoice-detail .label {
  color: var(--text-secondary);
  font-weight: 500;
  min-width: 80px;
}

.items-list {
  display: flex;
  flex-direction: column;
  gap: 4px;
}

.item-entry {
  display: flex;
  justify-content: space-between;
  font-size: 13px;
  color: var(--text-primary);
}
</style>
