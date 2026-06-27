<template>
  <div class="page-container">
    <div class="glass-card">
      <div class="glass-card-header">
        <h3>租赁管理</h3>
        <div class="header-controls">
          <el-input
            v-model="query.keyword"
            placeholder="搜索订单..."
            :prefix-icon="Search"
            clearable
            style="width: 200px"
            @keyup.enter="loadRentals"
            @clear="loadRentals"
          />
          <el-button type="primary" @click="loadRentals">
            <el-icon><Search /></el-icon> 搜索
          </el-button>
          <el-button type="primary" @click="$router.push('/rentals/create')">
            <el-icon><Plus /></el-icon> 新建订单
          </el-button>
        </div>
      </div>

      <!-- 状态标签页 -->
      <div class="tab-bar">
        <button
          v-for="tab in tabs"
          :key="tab.value"
          class="tab-btn"
          :class="{ active: activeTab === tab.value }"
          @click="handleTabChange(tab.value)"
        >{{ tab.label }}</button>
      </div>

      <div class="glass-card-body">
        <el-table :data="rentals" v-loading="loading" style="width: 100%" empty-text="暂无订单">
          <el-table-column prop="order_no" label="订单号" min-width="160" />
          <el-table-column prop="username" label="用户" min-width="100" />
          <el-table-column prop="plate_number" label="车辆" min-width="110" />
          <el-table-column prop="start_date" label="开始日期" min-width="110" />
          <el-table-column prop="end_date" label="结束日期" min-width="110" />
          <el-table-column prop="status" label="状态" min-width="100">
            <template #default="{ row }">
              <el-tag :type="statusType(row.status)" effect="dark" size="small">{{ statusLabel(row.status) }}</el-tag>
            </template>
          </el-table-column>
          <el-table-column prop="total_cost" label="总费用" min-width="100" align="right">
            <template #default="{ row }">
              <span class="text-accent">¥{{ row.total_cost || 0 }}</span>
            </template>
          </el-table-column>
          <el-table-column label="操作" min-width="180">
            <template #default="{ row }">
              <el-button link type="primary" @click="$router.push(`/rentals/${row.id}`)">查看</el-button>
              <el-button v-if="authStore.isAdmin && row.status === 'pending'" link type="success" @click="handlePickup(row.id)">取车</el-button>
              <el-button v-if="authStore.isAdmin && row.status === 'active'" link type="warning" @click="handleReturn(row.id)">还车</el-button>
            </template>
          </el-table-column>
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
      </div>
    </div>
  </div>
</template>

<script setup>
import { ref, reactive, onMounted } from 'vue'
import { useAuthStore } from '@/stores/auth'
import { getRentalList, pickupVehicle, returnVehicle } from '@/api/rental'
import { ElMessage } from 'element-plus'
import { Search, Plus } from '@element-plus/icons-vue'

const authStore = useAuthStore()
const loading = ref(false)
const rentals = ref([])
const total = ref(0)
const activeTab = ref('all')

const tabs = [
  { value: 'all', label: '全部' },
  { value: 'pending', label: '待处理' },
  { value: 'active', label: '进行中' },
  { value: 'completed', label: '已完成' },
]

const query = reactive({
  page: 1,
  page_size: 20,
  keyword: '',
  status: '',
})

function statusType(status) {
  const map = {
    'pending': 'warning',
    'active': 'success',
    'completed': 'info',
    'cancelled': 'danger',
    'overdue': 'danger',
  }
  return map[(status || '').toLowerCase()] || 'info'
}

function statusLabel(status) {
  const map = {
    'pending': '待处理',
    'active': '进行中',
    'completed': '已完成',
    'cancelled': '已取消',
    'overdue': '已逾期',
  }
  return map[(status || '').toLowerCase()] || status
}

async function loadRentals() {
  loading.value = true
  try {
    const params = { ...query }
    if (!params.keyword) delete params.keyword
    if (!params.status) delete params.status
    // 非管理员只能看自己的订单
    if (!authStore.isAdmin) {
      params.user_id = authStore.uid
    }
    const res = await getRentalList(params)
    if (res.error === 0) {
      rentals.value = res.list || res.orders || []
      total.value = res.total || 0
    }
  } catch {
    ElMessage.error('加载订单失败')
  } finally {
    loading.value = false
  }
}

function handleTabChange(tab) {
  activeTab.value = tab
  query.page = 1
  query.status = tab === 'all' ? '' : tab
  loadRentals()
}

async function handlePickup(orderId) {
  try {
    const res = await pickupVehicle({ order_id: orderId })
    if (res.error === 0) {
      ElMessage.success('车辆已取出')
      loadRentals()
    } else {
      ElMessage.error('取车失败')
    }
  } catch {
    ElMessage.error('取车失败')
  }
}

async function handleReturn(orderId) {
  try {
    const res = await returnVehicle({ order_id: orderId })
    if (res.error === 0) {
      ElMessage.success('车辆已归还')
      loadRentals()
    } else {
      ElMessage.error('还车失败')
    }
  } catch {
    ElMessage.error('还车失败')
  }
}

function handlePageChange(page) {
  query.page = page
  loadRentals()
}

onMounted(loadRentals)
</script>

<style scoped>
.header-controls {
  display: flex;
  gap: 10px;
  align-items: center;
  flex-wrap: wrap;
}

.tab-bar {
  display: flex;
  gap: 4px;
  padding: 0 24px;
  border-bottom: 1px solid var(--border);
}

.tab-btn {
  padding: 12px 16px;
  font-size: 13px;
  color: var(--text-secondary);
  background: transparent;
  border: none;
  border-bottom: 2px solid transparent;
  cursor: pointer;
  transition: all 0.2s var(--ease);
  font-family: var(--font-body);
}

.tab-btn:hover {
  color: var(--text-primary);
}

.tab-btn.active {
  color: var(--accent);
  border-bottom-color: var(--accent);
}

.pagination-wrapper {
  display: flex;
  justify-content: center;
  padding: 20px 0 8px;
}
</style>
