<template>
  <div class="page-container">
    <div class="page-header">
      <h2>收费管理</h2>
      <p>管理收费记录</p>
    </div>

    <div class="search-bar">
      <el-select v-model="query.status" placeholder="全部状态" clearable style="width: 140px" @change="loadList">
        <el-option label="待支付" value="pending" />
        <el-option label="已完成" value="completed" />
        <el-option label="失败" value="failed" />
        <el-option label="已退款" value="refunded" />
      </el-select>
      <el-select v-model="query.type" placeholder="全部类型" clearable style="width: 140px" @change="loadList">
        <el-option label="租赁费" value="rental" />
        <el-option label="押金" value="deposit" />
        <el-option label="罚款" value="penalty" />
        <el-option label="退款" value="refund" />
      </el-select>
      <el-button type="primary" @click="loadList">
        <el-icon><Search /></el-icon> 搜索
      </el-button>
      <el-button type="primary" @click="showCreateDialog" style="margin-left: auto">
        <el-icon><Plus /></el-icon> 新建收费
      </el-button>
    </div>

    <el-card>
      <el-table :data="payments" v-loading="loading" style="width: 100%" empty-text="暂无收费记录">
        <el-table-column prop="id" label="ID" min-width="60" />
        <el-table-column prop="order_no" label="订单号" min-width="150" />
        <el-table-column prop="amount" label="金额" min-width="100">
          <template #default="{ row }">¥{{ row.amount }}</template>
        </el-table-column>
        <el-table-column prop="type" label="类型" min-width="80">
          <template #default="{ row }">
            <el-tag>{{ row.type }}</el-tag>
          </template>
        </el-table-column>
        <el-table-column prop="method" label="支付方式" min-width="100" />
        <el-table-column prop="status" label="状态" min-width="100">
          <template #default="{ row }">
            <el-tag :type="paymentStatusType(row.status)">{{ row.status }}</el-tag>
          </template>
        </el-table-column>
        <el-table-column prop="created_at" label="日期" min-width="110" />
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
    <el-dialog v-model="dialogVisible" title="新建收费" width="480px">
      <el-form ref="createFormRef" :model="createForm" :rules="createRules" label-position="top">
        <el-form-item label="订单ID" prop="order_id">
          <el-input-number v-model="createForm.order_id" :min="1" style="width: 100%" />
        </el-form-item>
        <el-form-item label="金额 (CNY)" prop="amount">
          <el-input-number v-model="createForm.amount" :min="0" :precision="2" style="width: 100%" />
        </el-form-item>
        <el-form-item label="类型" prop="type">
          <el-select v-model="createForm.type" placeholder="请选择类型" style="width: 100%">
            <el-option label="租赁费" value="rental" />
            <el-option label="押金" value="deposit" />
            <el-option label="罚款" value="penalty" />
          </el-select>
        </el-form-item>
        <el-form-item label="支付方式" prop="method">
          <el-select v-model="createForm.method" placeholder="请选择支付方式" style="width: 100%">
            <el-option label="现金" value="cash" />
            <el-option label="微信支付" value="wechat" />
            <el-option label="支付宝" value="alipay" />
            <el-option label="银行转账" value="bank_transfer" />
          </el-select>
        </el-form-item>
        <el-form-item label="备注">
          <el-input v-model="createForm.remark" placeholder="选填" />
        </el-form-item>
      </el-form>
      <template #footer>
        <el-button @click="dialogVisible = false">取消</el-button>
        <el-button type="primary" :loading="creating" @click="handleCreate">提交</el-button>
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
  order_id: [{ required: true, message: '请输入订单ID', trigger: 'blur' }],
  amount: [{ required: true, message: '请输入金额', trigger: 'blur' }],
  type: [{ required: true, message: '请选择类型', trigger: 'change' }],
  method: [{ required: true, message: '请选择支付方式', trigger: 'change' }],
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
      ElMessage.success('收费记录已创建')
      dialogVisible.value = false
      loadList()
    } else {
      ElMessage.error('创建收费记录失败')
    }
  } catch {
    ElMessage.error('创建收费记录失败')
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
    ElMessage.error('加载收费记录失败')
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
