<template>
  <div class="page-container">
    <div class="page-header">
      <h2>Payments</h2>
      <p>Manage payment records</p>
    </div>

    <div class="search-bar">
      <el-select v-model="query.status" placeholder="All Status" clearable style="width: 140px" @change="loadList">
        <el-option label="Pending" value="pending" />
        <el-option label="Completed" value="completed" />
        <el-option label="Failed" value="failed" />
        <el-option label="Refunded" value="refunded" />
      </el-select>
      <el-select v-model="query.type" placeholder="All Types" clearable style="width: 140px" @change="loadList">
        <el-option label="Rental" value="rental" />
        <el-option label="Deposit" value="deposit" />
        <el-option label="Penalty" value="penalty" />
        <el-option label="Refund" value="refund" />
      </el-select>
      <el-button type="primary" @click="loadList">
        <el-icon><Search /></el-icon> Search
      </el-button>
      <el-button type="primary" @click="showCreateDialog" style="margin-left: auto">
        <el-icon><Plus /></el-icon> New Payment
      </el-button>
    </div>

    <el-card>
      <el-table :data="payments" v-loading="loading" style="width: 100%" empty-text="No payments found">
        <el-table-column prop="id" label="ID" min-width="60" />
        <el-table-column prop="order_no" label="Order No." min-width="150" />
        <el-table-column prop="amount" label="Amount" min-width="100">
          <template #default="{ row }">¥{{ row.amount }}</template>
        </el-table-column>
        <el-table-column prop="type" label="Type" min-width="80">
          <template #default="{ row }">
            <el-tag>{{ row.type }}</el-tag>
          </template>
        </el-table-column>
        <el-table-column prop="method" label="Method" min-width="100" />
        <el-table-column prop="status" label="Status" min-width="100">
          <template #default="{ row }">
            <el-tag :type="paymentStatusType(row.status)">{{ row.status }}</el-tag>
          </template>
        </el-table-column>
        <el-table-column prop="created_at" label="Date" min-width="110" />
      </el-table>

      <div class="pagination-wrapper" v-if="total > query.page_size">
        <el-pagination
          background
          layout="prev, pager, next"
          :total="total"
          :page-size="query.page_size"
          :current-page="query.page"
          @current-change="handlePageChange"
        />
      </div>
    </el-card>

    <!-- Create Payment Dialog -->
    <el-dialog v-model="dialogVisible" title="New Payment" width="480px">
      <el-form ref="createFormRef" :model="createForm" :rules="createRules" label-position="top">
        <el-form-item label="Order ID" prop="order_id">
          <el-input-number v-model="createForm.order_id" :min="1" style="width: 100%" />
        </el-form-item>
        <el-form-item label="Amount (CNY)" prop="amount">
          <el-input-number v-model="createForm.amount" :min="0" :precision="2" style="width: 100%" />
        </el-form-item>
        <el-form-item label="Type" prop="type">
          <el-select v-model="createForm.type" placeholder="Select type" style="width: 100%">
            <el-option label="Rental" value="rental" />
            <el-option label="Deposit" value="deposit" />
            <el-option label="Penalty" value="penalty" />
          </el-select>
        </el-form-item>
        <el-form-item label="Method" prop="method">
          <el-select v-model="createForm.method" placeholder="Select method" style="width: 100%">
            <el-option label="Cash" value="cash" />
            <el-option label="WeChat Pay" value="wechat" />
            <el-option label="Alipay" value="alipay" />
            <el-option label="Bank Transfer" value="bank_transfer" />
          </el-select>
        </el-form-item>
        <el-form-item label="Remark">
          <el-input v-model="createForm.remark" placeholder="Optional remark" />
        </el-form-item>
      </el-form>
      <template #footer>
        <el-button @click="dialogVisible = false">Cancel</el-button>
        <el-button type="primary" :loading="creating" @click="handleCreate">Submit</el-button>
      </template>
    </el-dialog>
  </div>
</template>

<script setup>
import { ref, reactive, onMounted } from 'vue'
import { getPaymentList, createPayment } from '@/api/finance'
import { ElMessage } from 'element-plus'
import { Search, Plus } from '@element-plus/icons-vue'

const loading = ref(false)
const payments = ref([])
const total = ref(0)
const dialogVisible = ref(false)
const creating = ref(false)
const createFormRef = ref(null)

const query = reactive({
  page: 1,
  page_size: 20,
  status: '',
  type: '',
})

const createForm = reactive({
  order_id: undefined,
  amount: 0,
  type: 'rental',
  method: 'wechat',
  remark: '',
})

const createRules = {
  order_id: [{ required: true, message: 'Order ID is required', trigger: 'blur' }],
  amount: [{ required: true, message: 'Amount is required', trigger: 'blur' }],
  type: [{ required: true, message: 'Type is required', trigger: 'change' }],
  method: [{ required: true, message: 'Method is required', trigger: 'change' }],
}

function paymentStatusType(status) {
  const map = { 'pending': 'warning', 'completed': 'success', 'failed': 'danger', 'refunded': 'info' }
  return map[(status || '').toLowerCase()] || 'info'
}

function showCreateDialog() {
  createForm.order_id = undefined
  createForm.amount = 0
  createForm.type = 'rental'
  createForm.method = 'wechat'
  createForm.remark = ''
  dialogVisible.value = true
}

async function handleCreate() {
  const valid = await createFormRef.value.validate().catch(() => false)
  if (!valid) return
  creating.value = true
  try {
    const res = await createPayment(createForm)
    if (res.error === 0) {
      ElMessage.success('Payment created')
      dialogVisible.value = false
      loadList()
    } else {
      ElMessage.error('Failed to create payment')
    }
  } catch {
    ElMessage.error('Failed to create payment')
  } finally {
    creating.value = false
  }
}

async function loadList() {
  loading.value = true
  try {
    const params = { ...query }
    if (!params.status) delete params.status
    if (!params.type) delete params.type
    const res = await getPaymentList(params)
    if (res.error === 0) {
      payments.value = res.list || res.payments || []
      total.value = res.total || 0
    }
  } catch {
    ElMessage.error('Failed to load payments')
  } finally {
    loading.value = false
  }
}

function handlePageChange(page) {
  query.page = page
  loadList()
}

onMounted(loadList)
</script>

<style scoped>
.search-bar {
  display: flex;
  gap: 12px;
  align-items: center;
  margin-bottom: 20px;
  flex-wrap: wrap;
}

.pagination-wrapper {
  display: flex;
  justify-content: center;
  margin-top: 20px;
}
</style>
