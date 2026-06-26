<template>
  <div class="page-container">
    <div class="page-header">
      <h2>Invoices</h2>
      <p>Manage invoices for rental orders</p>
    </div>

    <div class="search-bar">
      <el-button type="primary" @click="showGenerateDialog" style="margin-left: auto">
        <el-icon><Plus /></el-icon> Generate Invoice
      </el-button>
    </div>

    <el-card>
      <el-table :data="invoices" v-loading="loading" style="width: 100%" empty-text="No invoices found">
        <el-table-column prop="id" label="Invoice No." min-width="100" />
        <el-table-column prop="order_no" label="Order No." min-width="160" />
        <el-table-column prop="amount" label="Amount" min-width="100">
          <template #default="{ row }">¥{{ row.amount }}</template>
        </el-table-column>
        <el-table-column prop="status" label="Status" min-width="100">
          <template #default="{ row }">
            <el-tag :type="row.status === 'generated' ? 'success' : 'info'">{{ row.status }}</el-tag>
          </template>
        </el-table-column>
        <el-table-column prop="created_at" label="Date" min-width="120" />
        <el-table-column label="Actions" min-width="100">
          <template #default="{ row }">
            <el-button link type="primary" @click="viewDetail(row.id)">View</el-button>
          </template>
        </el-table-column>
      </el-table>
    </el-card>

    <!-- Generate Invoice Dialog -->
    <el-dialog v-model="dialogVisible" title="Generate Invoice" width="400px">
      <el-form ref="formRef" :model="form" :rules="rules" label-position="top">
        <el-form-item label="Order ID" prop="order_id">
          <el-input-number v-model="form.order_id" :min="1" style="width: 100%" />
        </el-form-item>
      </el-form>
      <template #footer>
        <el-button @click="dialogVisible = false">Cancel</el-button>
        <el-button type="primary" :loading="generating" @click="handleGenerate">Generate</el-button>
      </template>
    </el-dialog>

    <!-- Invoice Detail Dialog -->
    <el-dialog v-model="detailVisible" title="Invoice Detail" width="500px">
      <div v-if="invoiceDetail" class="invoice-detail">
        <div class="invoice-row"><span>Invoice No.:</span><span>{{ invoiceDetail.id }}</span></div>
        <div class="invoice-row"><span>Order No.:</span><span>{{ invoiceDetail.order_no }}</span></div>
        <div class="invoice-row"><span>Amount:</span><span>¥{{ invoiceDetail.amount }}</span></div>
        <div class="invoice-row"><span>Status:</span><span>{{ invoiceDetail.status }}</span></div>
        <div class="invoice-row"><span>Date:</span><span>{{ invoiceDetail.created_at }}</span></div>
      </div>
    </el-dialog>
  </div>
</template>

<script setup>
import { ref, reactive, onMounted } from 'vue'
import { getInvoiceDetail, generateInvoice } from '@/api/finance'
import { ElMessage } from 'element-plus'
import { Plus } from '@element-plus/icons-vue'

const loading = ref(false)
const invoices = ref([])
const dialogVisible = ref(false)
const detailVisible = ref(false)
const generating = ref(false)
const formRef = ref(null)
const invoiceDetail = ref(null)

const form = reactive({ order_id: undefined })
const rules = {
  order_id: [{ required: true, message: 'Order ID is required', trigger: 'blur' }],
}

function showGenerateDialog() {
  form.order_id = undefined
  dialogVisible.value = true
}

async function handleGenerate() {
  const valid = await formRef.value.validate().catch(() => false)
  if (!valid) return
  generating.value = true
  try {
    const res = await generateInvoice(form)
    if (res.error === 0) {
      ElMessage.success('Invoice generated')
      dialogVisible.value = false
    } else {
      ElMessage.error('Failed to generate invoice')
    }
  } catch {
    ElMessage.error('Failed to generate invoice')
  } finally {
    generating.value = false
  }
}

async function viewDetail(id) {
  try {
    const res = await getInvoiceDetail({ id })
    if (res.error === 0) {
      invoiceDetail.value = res.invoice || res
      detailVisible.value = true
    }
  } catch {
    ElMessage.error('Failed to load invoice detail')
  }
}

// Note: The API spec doesn't list a GET /invoice/list endpoint.
// This page uses a placeholder approach. In production, add a list endpoint.
onMounted(async () => {
  // Placeholder: no list endpoint in spec, show empty state
  loading.value = false
})
</script>

<style scoped>
.search-bar {
  display: flex;
  gap: 12px;
  align-items: center;
  margin-bottom: 20px;
  flex-wrap: wrap;
}

.invoice-detail .invoice-row {
  display: flex;
  justify-content: space-between;
  padding: 10px 0;
  border-bottom: 1px solid var(--border-light);
  font-size: 14px;
}

.invoice-detail .invoice-row:last-child {
  border-bottom: none;
}

.invoice-detail .invoice-row span:first-child {
  color: var(--text-secondary);
  font-weight: 500;
}
</style>
