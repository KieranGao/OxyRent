<template>
  <div class="page-container">
    <div class="page-header">
      <h2>工作台</h2>
      <p>欢迎回来，{{ authStore.username }}</p>
    </div>

    <!-- Customer Dashboard -->
    <template v-if="authStore.isCustomer">
      <div class="stat-grid anim-stagger">
        <div class="stat-card stat-card-wide">
          <div class="stat-icon gold">
            <el-icon :size="20"><Wallet /></el-icon>
          </div>
          <div class="stat-value stat-value-large">{{ formatMoney(authStore.balance) }}</div>
          <div class="stat-label">账户余额</div>
        </div>
        <div class="stat-card">
          <div class="stat-icon green">
            <el-icon :size="20"><Van /></el-icon>
          </div>
          <div class="stat-value">{{ stats.available_vehicles }}</div>
          <div class="stat-label">可用车辆</div>
        </div>
        <div class="stat-card">
          <div class="stat-icon amber">
            <el-icon :size="20"><Document /></el-icon>
          </div>
          <div class="stat-value">{{ stats.active_orders }}</div>
          <div class="stat-label">进行中订单</div>
        </div>
      </div>
    </template>

    <!-- Staff Dashboard -->
    <template v-else-if="authStore.isStaff">
      <div class="stat-grid anim-stagger">
        <div class="stat-card">
          <div class="stat-icon green">
            <el-icon :size="20"><Van /></el-icon>
          </div>
          <div class="stat-value">{{ stats.available_vehicles }}</div>
          <div class="stat-label">可用车辆</div>
        </div>
        <div class="stat-card">
          <div class="stat-icon amber">
            <el-icon :size="20"><Document /></el-icon>
          </div>
          <div class="stat-value">{{ stats.active_orders }}</div>
          <div class="stat-label">进行中订单</div>
        </div>
      </div>
    </template>

    <!-- Admin Dashboard -->
    <template v-else>
      <div class="stat-grid anim-stagger">
        <div class="stat-card">
          <div class="stat-icon blue">
            <el-icon :size="20"><User /></el-icon>
          </div>
          <div class="stat-value">{{ stats.total_users }}</div>
          <div class="stat-label">用户总数</div>
        </div>
        <div class="stat-card">
          <div class="stat-icon green">
            <el-icon :size="20"><Van /></el-icon>
          </div>
          <div class="stat-value">{{ stats.available_vehicles }}</div>
          <div class="stat-label">可用车辆</div>
        </div>
        <div class="stat-card">
          <div class="stat-icon amber">
            <el-icon :size="20"><Document /></el-icon>
          </div>
          <div class="stat-value">{{ stats.active_orders }}</div>
          <div class="stat-label">进行中订单</div>
        </div>
        <div class="stat-card">
          <div class="stat-icon gold">
            <el-icon :size="20"><Wallet /></el-icon>
          </div>
          <div class="stat-value">{{ formatMoney(stats.monthly_revenue) }}</div>
          <div class="stat-label">本月收入</div>
        </div>
      </div>
    </template>

    <!-- Quick Actions + Recent Orders -->
    <div class="dashboard-grid">
      <!-- Quick Actions -->
      <div class="content-card">
        <div class="content-card-header">
          <h3>快捷操作</h3>
        </div>
        <div class="content-card-body">
          <div class="action-list">
            <button class="action-item" @click="$router.push('/rentals/create')">
              <div class="action-icon gold">
                <el-icon><Plus /></el-icon>
              </div>
              <div>
                <div class="action-title">新建订单</div>
                <div class="action-desc">创建租赁订单</div>
              </div>
            </button>
            <button class="action-item" @click="$router.push('/rentals')">
              <div class="action-icon amber">
                <el-icon><Document /></el-icon>
              </div>
              <div>
                <div class="action-title">查看订单</div>
                <div class="action-desc">管理租赁订单</div>
              </div>
            </button>
            <button v-if="authStore.isStaff || authStore.isAdmin" class="action-item" @click="$router.push('/vehicles')">
              <div class="action-icon blue">
                <el-icon><Van /></el-icon>
              </div>
              <div>
                <div class="action-title">车辆管理</div>
                <div class="action-desc">查看车辆信息</div>
              </div>
            </button>
            <button v-if="authStore.isStaff || authStore.isAdmin" class="action-item" @click="$router.push('/topup')">
              <div class="action-icon green">
                <el-icon><Wallet /></el-icon>
              </div>
              <div>
                <div class="action-title">充值客户</div>
                <div class="action-desc">为客户充值余额</div>
              </div>
            </button>
            <button v-if="authStore.isAdmin" class="action-item" @click="$router.push('/statistics')">
              <div class="action-icon amber">
                <el-icon><DataLine /></el-icon>
              </div>
              <div>
                <div class="action-title">统计报表</div>
                <div class="action-desc">查看数据分析</div>
              </div>
            </button>
          </div>
        </div>
      </div>

      <!-- Recent Orders -->
      <div class="content-card">
        <div class="content-card-header">
          <h3>最近订单</h3>
        </div>
        <div class="content-card-body" style="padding: 0;">
          <el-table :data="recentOrders" style="width: 100%" empty-text="暂无订单">
            <el-table-column prop="order_no" label="订单号" min-width="140" />
            <el-table-column prop="plate_number" label="车辆" min-width="100" />
            <el-table-column prop="start_date" label="开始日期" min-width="100" />
            <el-table-column prop="status" label="状态" min-width="90">
              <template #default="{ row }">
                <el-tag :type="statusType(row.status)" size="small">{{ row.status }}</el-tag>
              </template>
            </el-table-column>
            <el-table-column prop="total_cost" label="费用" min-width="100">
              <template #default="{ row }">
                {{ formatMoney(row.total_cost) }}
              </template>
            </el-table-column>
          </el-table>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup>
import { ref, onMounted } from 'vue'
import { useAuthStore } from '@/stores/auth'
import { getStatsOverview } from '@/api/finance'
import { getRentalList } from '@/api/rental'
import { getBalance } from '@/api/user'
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
  // Fetch balance for customer and staff
  if (authStore.isCustomer || authStore.isStaff) {
    try {
      const res = await getBalance()
      if (res.error === 0) {
        authStore.setBalance(res.balance || 0)
      }
    } catch {
      // Balance unavailable
    }
  }

  // 只有管理员获取统计数据
  if (authStore.isAdmin) {
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
      // Stats unavailable
    }
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
.dashboard-grid {
  display: grid;
  grid-template-columns: 340px 1fr;
  gap: 24px;
  align-items: start;
}

.stat-card-wide {
  grid-column: 1 / -1;
}

.stat-value-large {
  font-size: 36px !important;
}

/* ===== Action List ===== */
.action-list {
  display: flex;
  flex-direction: column;
  gap: 4px;
}

.action-item {
  display: flex;
  align-items: center;
  gap: 14px;
  padding: 14px 12px;
  border-radius: var(--radius-md);
  border: none;
  background: transparent;
  cursor: pointer;
  width: 100%;
  text-align: left;
  transition: all 0.2s var(--ease);
}

.action-item:hover {
  background: rgba(255, 255, 255, 0.04);
}

.action-icon {
  width: 36px;
  height: 36px;
  border-radius: var(--radius-sm);
  display: flex;
  align-items: center;
  justify-content: center;
  flex-shrink: 0;
  font-size: 16px;
}

.action-icon.gold { background: var(--accent-muted); color: var(--accent); }
.action-icon.blue { background: rgba(96, 165, 250, 0.1); color: #60a5fa; }
.action-icon.green { background: rgba(74, 222, 128, 0.1); color: #4ade80; }
.action-icon.amber { background: rgba(251, 191, 36, 0.1); color: #fbbf24; }

.action-title {
  font-size: 13px;
  font-weight: 500;
  color: var(--text-primary);
}

.action-desc {
  font-size: 11px;
  color: var(--text-tertiary);
  margin-top: 2px;
}

/* ===== Responsive ===== */
@media (max-width: 900px) {
  .dashboard-grid {
    grid-template-columns: 1fr;
  }
}
</style>