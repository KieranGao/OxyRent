<template>
  <div class="page-container">
    <div class="page-header">
      <h2>我的余额</h2>
      <p>查看账户余额和充值消费记录</p>
    </div>

    <!-- Balance Card -->
    <div class="balance-card">
      <div class="balance-card-inner">
        <div class="balance-info">
          <div class="balance-label">当前余额</div>
          <div class="balance-value">{{ formatMoney(balance) }}</div>
        </div>
        <el-button v-if="authStore.isStaff || authStore.isAdmin" type="primary" @click="$router.push('/topup')">
          为客户充值
        </el-button>
      </div>
    </div>

    <!-- Balance Records -->
    <el-card>
      <template #header>
        <div class="card-header">
          <span>收支明细</span>
        </div>
      </template>
      <el-table :data="records" v-loading="loading" style="width: 100%" empty-text="暂无记录">
        <el-table-column prop="id" label="ID" min-width="60" />
        <el-table-column prop="type" label="类型" min-width="80">
          <template #default="{ row }">
            <el-tag :type="row.type === 'topup' ? 'success' : 'info'">
              {{ row.type === 'topup' ? '充值' : '消费' }}
            </el-tag>
          </template>
        </el-table-column>
        <el-table-column prop="amount" label="金额" min-width="100">
          <template #default="{ row }">
            <span :style="{ color: row.type === 'topup' ? 'var(--accent)' : 'var(--text-primary)' }">
              {{ row.type === 'topup' ? '+' : '-' }}{{ formatMoney(row.amount) }}
            </span>
          </template>
        </el-table-column>
        <el-table-column prop="remark" label="备注" min-width="160" />
        <el-table-column prop="created_at" label="时间" min-width="160" />
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
  </div>
</template>

<script setup>
import { ref, reactive, onMounted } from 'vue'
import { useAuthStore } from '@/stores/auth'
import { getBalance, getBalanceRecords } from '@/api/user'
import { ElMessage } from 'element-plus'

const authStore = useAuthStore()
const balance = ref(0)
const loading = ref(false)
const records = ref([])
const total = ref(0)

const query = reactive({
  page: 1,
  page_size: 20,
})

function formatMoney(val) {
  return (val || 0).toLocaleString('zh-CN', { style: 'currency', currency: 'CNY' })
}

async function loadBalance() {
  try {
    const res = await getBalance()
    if (res.error === 0) {
      balance.value = res.balance || 0
      authStore.setBalance(balance.value)
    }
  } catch {
    ElMessage.error('获取余额失败')
  }
}

async function loadRecords() {
  loading.value = true
  try {
    const res = await getBalanceRecords(query)
    if (res.error === 0) {
      records.value = res.list || res.records || []
      total.value = res.total || 0
    }
  } catch {
    ElMessage.error('获取收支记录失败')
  } finally {
    loading.value = false
  }
}

function handlePageChange(page) {
  query.page = page
  loadRecords()
}

onMounted(() => {
  loadBalance()
  loadRecords()
})
</script>

<style scoped>
.balance-card {
  margin-bottom: 24px;
}

.balance-card-inner {
  background: linear-gradient(135deg, var(--accent-muted), rgba(255, 255, 255, 0.02));
  border: 1px solid var(--border);
  border-radius: var(--radius-lg);
  padding: 32px;
  display: flex;
  align-items: center;
  justify-content: space-between;
}

.balance-label {
  font-size: 14px;
  color: var(--text-secondary);
  margin-bottom: 8px;
}

.balance-value {
  font-size: 36px;
  font-weight: 600;
  color: var(--text-primary);
  font-family: var(--font-display);
}

.card-header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  font-weight: 500;
}

.pagination-wrapper {
  display: flex;
  justify-content: center;
  margin-top: 20px;
}
</style>
