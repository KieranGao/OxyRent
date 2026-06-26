<template>
  <div class="page-container">
    <div class="page-header">
      <h2>Rentals</h2>
      <p>Manage rental orders</p>
    </div>

    <!-- Status Tabs -->
    <el-tabs v-model="activeTab" @tab-change="handleTabChange">
      <el-tab-pane label="All" name="all" />
      <el-tab-pane label="Pending" name="pending" />
      <el-tab-pane label="Active" name="active" />
      <el-tab-pane label="Completed" name="completed" />
    </el-tabs>

    <div class="search-bar">
      <el-input
        v-model="query.keyword"
        placeholder="Search orders..."
        :prefix-icon="Search"
        clearable
        style="width: 240px"
        @keyup.enter="loadRentals"
        @clear="loadRentals"
      />
      <el-button type="primary" @click="loadRentals">
        <el-icon><Search /></el-icon> Search
      </el-button>
      <el-button type="primary" @click="$router.push('/rentals/create')" style="margin-left: auto">
        <el-icon><Plus /></el-icon> New Order
      </el-button>
    </div>

    <el-card>
      <el-table :data="rentals" v-loading="loading" style="width: 100%" empty-text="No orders found">
        <el-table-column prop="order_no" label="Order No." min-width="160" />
        <el-table-column prop="username" label="User" min-width="100" />
        <el-table-column prop="plate_number" label="Vehicle" min-width="110" />
        <el-table-column prop="start_date" label="Start Date" min-width="110" />
        <el-table-column prop="end_date" label="End Date" min-width="110" />
        <el-table-column prop="status" label="Status" min-width="100">
          <template #default="{ row }">
            <el-tag :type="statusType(row.status)">{{ row.status }}</el-tag>
          </template>
        </el-table-column>
        <el-table-column prop="total_cost" label="Total Cost" min-width="100">
          <template #default="{ row }">
            ¥{{ row.total_cost || 0 }}
          </template>
        </el-table-column>
        <el-table-column label="Actions" min-width="180">
          <template #default="{ row }">
            <el-button link type="primary" @click="$router.push(`/rentals/${row.id}`)">View</el-button>
            <el-button v-if="authStore.isAdmin && row.status === 'pending'" link type="success" @click="handlePickup(row.id)">Pickup</el-button>
            <el-button v-if="authStore.isAdmin && row.status === 'active'" link type="warning" @click="handleReturn(row.id)">Return</el-button>
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
    </el-card>
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

async function loadRentals() {
  loading.value = true
  try {
    const params = { ...query }
    if (!params.keyword) delete params.keyword
    if (!params.status) delete params.status
    const res = await getRentalList(params)
    if (res.error === 0) {
      rentals.value = res.list || res.orders || []
      total.value = res.total || 0
    }
  } catch {
    ElMessage.error('Failed to load rentals')
  } finally {
    loading.value = false
  }
}

function handleTabChange(tab) {
  query.page = 1
  query.status = tab === 'all' ? '' : tab
  loadRentals()
}

async function handlePickup(orderId) {
  try {
    const res = await pickupVehicle({ order_id: orderId })
    if (res.error === 0) {
      ElMessage.success('Vehicle picked up')
      loadRentals()
    } else {
      ElMessage.error('Pickup failed')
    }
  } catch {
    ElMessage.error('Pickup failed')
  }
}

async function handleReturn(orderId) {
  try {
    const res = await returnVehicle({ order_id: orderId })
    if (res.error === 0) {
      ElMessage.success('Vehicle returned')
      loadRentals()
    } else {
      ElMessage.error('Return failed')
    }
  } catch {
    ElMessage.error('Return failed')
  }
}

function handlePageChange(page) {
  query.page = page
  loadRentals()
}

onMounted(loadRentals)
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
