<template>
  <div class="page-container">
    <div class="page-header">
      <h2>Dashboard</h2>
      <p>Welcome back, {{ authStore.username }}</p>
    </div>

    <!-- Stat Cards -->
    <div class="stat-cards anim-stagger">
      <div class="stat-card">
        <div class="stat-icon" style="background: var(--color-primary-bg)">
          <el-icon :size="24" color="var(--color-primary)"><User /></el-icon>
        </div>
        <div class="stat-info">
          <div class="stat-value">{{ stats.total_users }}</div>
          <div class="stat-label">Total Users</div>
        </div>
      </div>
      <div class="stat-card">
        <div class="stat-icon" style="background: var(--color-success-bg)">
          <el-icon :size="24" color="var(--color-success)"><Van /></el-icon>
        </div>
        <div class="stat-info">
          <div class="stat-value">{{ stats.available_vehicles }}</div>
          <div class="stat-label">Available Vehicles</div>
        </div>
      </div>
      <div class="stat-card">
        <div class="stat-icon" style="background: var(--color-warning-bg)">
          <el-icon :size="24" color="var(--color-warning)"><Document /></el-icon>
        </div>
        <div class="stat-info">
          <div class="stat-value">{{ stats.active_orders }}</div>
          <div class="stat-label">Active Orders</div>
        </div>
      </div>
      <div class="stat-card">
        <div class="stat-icon" style="background: var(--color-danger-bg)">
          <el-icon :size="24" color="var(--color-danger)"><Wallet /></el-icon>
        </div>
        <div class="stat-info">
          <div class="stat-value">{{ formatMoney(stats.monthly_revenue) }}</div>
          <div class="stat-label">Monthly Revenue</div>
        </div>
      </div>
    </div>

    <!-- Quick Actions -->
    <div class="section-title">Quick Actions</div>
    <div class="quick-actions anim-stagger">
      <el-button type="primary" @click="$router.push('/rentals/create')">
        <el-icon><Plus /></el-icon> New Order
      </el-button>
      <el-button v-if="authStore.isAdmin" @click="$router.push('/vehicles/add')">
        <el-icon><Plus /></el-icon> Add Vehicle
      </el-button>
      <el-button v-if="authStore.isAdmin" @click="$router.push('/payments')">
        <el-icon><Wallet /></el-icon> Payments
      </el-button>
      <el-button @click="$router.push('/statistics')">
        <el-icon><DataLine /></el-icon> Reports
      </el-button>
    </div>

    <!-- Recent Orders -->
    <div class="section-title">Recent Orders</div>
    <el-card>
      <el-table :data="recentOrders" style="width: 100%" empty-text="No recent orders">
        <el-table-column prop="order_no" label="Order No." min-width="160" />
        <el-table-column prop="username" label="User" min-width="100" />
        <el-table-column prop="plate_number" label="Plate" min-width="100" />
        <el-table-column prop="start_date" label="Start" min-width="110" />
        <el-table-column prop="end_date" label="End" min-width="110" />
        <el-table-column prop="status" label="Status" min-width="100">
          <template #default="{ row }">
            <el-tag :type="statusType(row.status)">{{ row.status }}</el-tag>
          </template>
        </el-table-column>
        <el-table-column prop="total_cost" label="Cost" min-width="100">
          <template #default="{ row }">
            {{ formatMoney(row.total_cost) }}
          </template>
        </el-table-column>
      </el-table>
    </el-card>
  </div>
</template>

<script setup>
import { ref, onMounted } from 'vue'
import { useAuthStore } from '@/stores/auth'
import { getStatsOverview } from '@/api/finance'
import { getRentalList } from '@/api/rental'
import { User, Van, Document, Wallet, Plus, DataLine } from '@element-plus/icons-vue'

const authStore = useAuthStore()

const stats = ref({
  total_users: 0,
  available_vehicles: 0,
  active_orders: 0,
  monthly_revenue: 0,
})

const recentOrders = ref([])

function formatMoney(val) {
  return (val || 0).toLocaleString('zh-CN', { style: 'currency', currency: 'CNY' })
}

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

onMounted(async () => {
  try {
    const res = await getStatsOverview()
    if (res.error === 0) {
      stats.value = {
        total_users: res.total_users || 0,
        available_vehicles: res.available_vehicles || 0,
        active_orders: res.active_orders || 0,
        monthly_revenue: res.monthly_revenue || 0,
      }
    }
  } catch {
    // Stats unavailable, use defaults
  }

  try {
    const res = await getRentalList({ page: 1, page_size: 10 })
    if (res.error === 0) {
      recentOrders.value = res.list || res.orders || []
    }
  } catch {
    // Orders unavailable
  }
})
</script>

<style scoped>
.stat-cards {
  display: grid;
  grid-template-columns: repeat(4, 1fr);
  gap: 20px;
  margin-bottom: 32px;
}

.stat-card {
  background: var(--bg-secondary);
  border: 1px solid var(--border-light);
  border-radius: var(--radius-md);
  padding: 24px;
  display: flex;
  align-items: center;
  gap: 16px;
  transition: all var(--transition-fast);
}

.stat-card:hover {
  box-shadow: var(--shadow-md);
  transform: translateY(-2px);
}

.stat-icon {
  width: 52px;
  height: 52px;
  border-radius: var(--radius-md);
  display: flex;
  align-items: center;
  justify-content: center;
  flex-shrink: 0;
}

.stat-value {
  font-size: 24px;
  font-weight: 700;
  color: var(--text-primary);
  line-height: 1.2;
}

.stat-label {
  font-size: 13px;
  color: var(--text-secondary);
  margin-top: 4px;
}

.section-title {
  font-size: 16px;
  font-weight: 600;
  color: var(--text-primary);
  margin-bottom: 16px;
}

.quick-actions {
  display: flex;
  gap: 12px;
  margin-bottom: 32px;
}
</style>
