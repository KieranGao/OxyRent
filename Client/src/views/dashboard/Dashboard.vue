<template>
  <div class="page-container">
    <!-- Greeting -->
    <div class="dash-greeting">
      <h2 class="greeting-text">
        {{ greetingText }}，<span class="text-accent">{{ authStore.username }}</span>
      </h2>
      <p class="greeting-sub">{{ todayStr }}</p>
    </div>

    <!-- Customer Dashboard -->
    <template v-if="authStore.isCustomer">
      <div class="bento-grid bento-3col anim-stagger">
        <div class="bento-cell span-2 hero-stat">
          <div class="hero-stat-icon">
            <el-icon :size="28"><Wallet /></el-icon>
          </div>
          <div class="hero-stat-value">{{ formatMoney(authStore.balance) }}</div>
          <div class="hero-stat-label">账户余额</div>
          <div class="hero-stat-glow"></div>
        </div>
        <div class="bento-cell">
          <div class="mini-stat">
            <div class="mini-stat-icon green"><el-icon :size="18"><Van /></el-icon></div>
            <div class="mini-stat-value">{{ stats.available_vehicles }}</div>
            <div class="mini-stat-label">可用车辆</div>
          </div>
        </div>
        <div class="bento-cell span-2">
          <div class="mini-stat">
            <div class="mini-stat-icon amber"><el-icon :size="18"><Document /></el-icon></div>
            <div class="mini-stat-value">{{ stats.active_orders }}</div>
            <div class="mini-stat-label">进行中订单</div>
          </div>
        </div>
      </div>
    </template>

    <!-- Staff Dashboard -->
    <template v-else-if="authStore.isStaff">
      <div class="bento-grid bento-4col anim-stagger">
        <div class="bento-cell span-2">
          <div class="mini-stat">
            <div class="mini-stat-icon green"><el-icon :size="18"><Van /></el-icon></div>
            <div class="mini-stat-value">{{ stats.available_vehicles }}</div>
            <div class="mini-stat-label">可用车辆</div>
          </div>
        </div>
        <div class="bento-cell span-2">
          <div class="mini-stat">
            <div class="mini-stat-icon amber"><el-icon :size="18"><Document /></el-icon></div>
            <div class="mini-stat-value">{{ stats.active_orders }}</div>
            <div class="mini-stat-label">进行中订单</div>
          </div>
        </div>
      </div>
    </template>

    <!-- Admin Dashboard -->
    <template v-else>
      <div class="bento-grid bento-6col anim-stagger">
        <!-- Hero: Month Revenue -->
        <div class="bento-cell span-2 row-2 hero-stat">
          <div class="hero-stat-icon">
            <el-icon :size="28"><Wallet /></el-icon>
          </div>
          <div class="hero-stat-value">{{ formatMoney(stats.month_revenue) }}</div>
          <div class="hero-stat-label">本月收入</div>
          <div class="hero-stat-glow"></div>
        </div>
        <!-- 4 Small Stats -->
        <div class="bento-cell">
          <div class="mini-stat">
            <div class="mini-stat-icon blue"><el-icon :size="18"><User /></el-icon></div>
            <div class="mini-stat-value">{{ stats.total_users }}</div>
            <div class="mini-stat-label">用户总数</div>
          </div>
        </div>
        <div class="bento-cell">
          <div class="mini-stat">
            <div class="mini-stat-icon green"><el-icon :size="18"><Van /></el-icon></div>
            <div class="mini-stat-value">{{ stats.available_vehicles }}</div>
            <div class="mini-stat-label">可用车辆</div>
          </div>
        </div>
        <div class="bento-cell">
          <div class="mini-stat">
            <div class="mini-stat-icon amber"><el-icon :size="18"><Document /></el-icon></div>
            <div class="mini-stat-value">{{ stats.active_orders }}</div>
            <div class="mini-stat-label">进行中订单</div>
          </div>
        </div>
        <div class="bento-cell">
          <div class="mini-stat">
            <div class="mini-stat-icon gold"><el-icon :size="18"><DataLine /></el-icon></div>
            <div class="mini-stat-value">{{ stats.completed_orders }}</div>
            <div class="mini-stat-label">已完成订单</div>
          </div>
        </div>
      </div>
    </template>

    <!-- Recent Orders + Quick Actions -->
    <div class="dash-bottom-grid">
      <!-- Recent Orders -->
      <div class="glass-card">
        <div class="glass-card-header">
          <h3>最近订单</h3>
          <el-button text type="primary" @click="$router.push('/rentals')">查看全部</el-button>
        </div>
        <div class="glass-card-body">
          <el-table :data="recentOrders" style="width: 100%" empty-text="暂无订单">
            <el-table-column prop="order_no" label="订单号" min-width="150" />
            <el-table-column prop="plate_number" label="车辆" min-width="100" />
            <el-table-column prop="start_date" label="开始日期" min-width="110" />
            <el-table-column prop="status" label="状态" min-width="90">
              <template #default="{ row }">
                <el-tag :type="statusType(row.status)" size="small" effect="dark">{{ statusLabel(row.status) }}</el-tag>
              </template>
            </el-table-column>
            <el-table-column prop="total_cost" label="费用" min-width="100" align="right">
              <template #default="{ row }">
                <span class="text-accent">{{ formatMoney(row.total_cost) }}</span>
              </template>
            </el-table-column>
          </el-table>
        </div>
      </div>

      <!-- Quick Actions -->
      <div class="glass-card actions-card">
        <div class="glass-card-header">
          <h3>快捷操作</h3>
        </div>
        <div class="glass-card-body padded">
          <div class="action-grid">
            <button class="action-tile" @click="$router.push('/rentals/create')">
              <div class="action-tile-icon gold"><el-icon><Plus /></el-icon></div>
              <span>新建订单</span>
            </button>
            <button class="action-tile" @click="$router.push('/rentals')">
              <div class="action-tile-icon amber"><el-icon><Document /></el-icon></div>
              <span>查看订单</span>
            </button>
            <button v-if="authStore.isStaff || authStore.isAdmin" class="action-tile" @click="$router.push('/vehicles')">
              <div class="action-tile-icon blue"><el-icon><Van /></el-icon></div>
              <span>车辆管理</span>
            </button>
            <button v-if="authStore.isStaff || authStore.isAdmin" class="action-tile" @click="$router.push('/topup')">
              <div class="action-tile-icon green"><el-icon><Wallet /></el-icon></div>
              <span>充值客户</span>
            </button>
            <button v-if="authStore.isAdmin" class="action-tile" @click="$router.push('/statistics')">
              <div class="action-tile-icon amber"><el-icon><DataLine /></el-icon></div>
              <span>统计报表</span>
            </button>
            <button v-if="authStore.isAdmin" class="action-tile" @click="$router.push('/users')">
              <div class="action-tile-icon blue"><el-icon><User /></el-icon></div>
              <span>用户管理</span>
            </button>
          </div>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup>
import { ref, computed, onMounted } from 'vue'
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
  completed_orders: 0,
  month_revenue: 0,
})

const recentOrders = ref([])

const greetingText = computed(() => {
  const h = new Date().getHours()
  if (h < 6) return '夜深了'
  if (h < 12) return '早上好'
  if (h < 18) return '下午好'
  return '晚上好'
})

const todayStr = computed(() => {
  const d = new Date()
  const weekdays = ['周日', '周一', '周二', '周三', '周四', '周五', '周六']
  return `${d.getFullYear()}年${d.getMonth() + 1}月${d.getDate()}日 ${weekdays[d.getDay()]}`
})

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

function statusLabel(status) {
  const map = {
    'pending': '待确认',
    'active': '进行中',
    'completed': '已完成',
    'cancelled': '已取消',
    'overdue': '已逾期',
  }
  return map[(status || '').toLowerCase()] || status
}

onMounted(async () => {
  if (authStore.isCustomer || authStore.isStaff) {
    try {
      const res = await getBalance()
      if (res.error === 0) {
        authStore.setBalance(res.balance || 0)
      }
    } catch {}
  }

  if (authStore.isAdmin) {
    try {
      const res = await getStatsOverview()
      if (res.error === 0) {
        stats.value = {
          total_users: res.total_users || 0,
          available_vehicles: res.available_vehicles || 0,
          active_orders: res.active_orders || 0,
          completed_orders: res.completed_orders || 0,
          month_revenue: res.month_revenue || 0,
        }
      }
    } catch {}
  }

  try {
    const res = await getRentalList({ page: 1, page_size: 10 })
    if (res.error === 0) {
      recentOrders.value = res.list || res.orders || []
    }
  } catch {}
})
</script>

<style scoped>
/* ===== Greeting ===== */
.dash-greeting {
  margin-bottom: 28px;
}

.greeting-text {
  font-family: var(--font-display);
  font-size: 28px;
  font-weight: 400;
  color: var(--text-primary);
  letter-spacing: 0.5px;
}

.greeting-sub {
  font-size: 13px;
  color: var(--text-tertiary);
  margin-top: 4px;
}

/* ===== Bento Grid ===== */
.bento-grid {
  display: grid;
  gap: 16px;
  margin-bottom: 24px;
}

.bento-3col { grid-template-columns: repeat(3, 1fr); }
.bento-4col { grid-template-columns: repeat(4, 1fr); }
.bento-6col { grid-template-columns: repeat(4, 1fr); }

.span-2 { grid-column: span 2; }
.row-2 { grid-row: span 2; }

.bento-cell {
  min-height: 0;
}

/* ===== Hero Stat ===== */
.hero-stat {
  background: var(--bg-surface);
  border: 1px solid var(--border);
  border-radius: var(--radius-lg);
  padding: 28px;
  display: flex;
  flex-direction: column;
  justify-content: center;
  position: relative;
  overflow: hidden;
  transition: border-color 0.3s var(--ease);
}

.hero-stat:hover {
  border-color: var(--border-accent);
}

.hero-stat-icon {
  width: 48px;
  height: 48px;
  border-radius: 12px;
  background: var(--accent-muted);
  color: var(--accent);
  display: flex;
  align-items: center;
  justify-content: center;
  margin-bottom: 20px;
}

.hero-stat-value {
  font-family: var(--font-body);
  font-size: clamp(32px, 4vw, 48px);
  font-weight: 300;
  color: var(--text-primary);
  letter-spacing: -0.02em;
  line-height: 1;
  margin-bottom: 8px;
}

.hero-stat-label {
  font-size: 13px;
  color: var(--text-secondary);
  letter-spacing: 1px;
  text-transform: uppercase;
  font-weight: 400;
}

.hero-stat-glow {
  position: absolute;
  top: -50%;
  right: -30%;
  width: 200px;
  height: 200px;
  background: radial-gradient(circle, rgba(200, 169, 110, 0.08) 0%, transparent 70%);
  pointer-events: none;
}

/* ===== Mini Stat ===== */
.mini-stat {
  background: var(--bg-surface);
  border: 1px solid var(--border);
  border-radius: var(--radius-md);
  padding: 20px;
  display: flex;
  flex-direction: column;
  gap: 12px;
  transition: border-color 0.25s var(--ease);
  height: 100%;
}

.mini-stat:hover {
  border-color: var(--border-hover);
}

.mini-stat-icon {
  width: 36px;
  height: 36px;
  border-radius: 8px;
  display: flex;
  align-items: center;
  justify-content: center;
}

.mini-stat-icon.blue { background: rgba(96, 165, 250, 0.1); color: #60a5fa; }
.mini-stat-icon.green { background: rgba(74, 222, 128, 0.1); color: #4ade80; }
.mini-stat-icon.amber { background: rgba(251, 191, 36, 0.1); color: #fbbf24; }
.mini-stat-icon.gold { background: var(--accent-muted); color: var(--accent); }

.mini-stat-value {
  font-family: var(--font-body);
  font-size: 28px;
  font-weight: 300;
  color: var(--text-primary);
  letter-spacing: -0.02em;
  line-height: 1;
}

.mini-stat-label {
  font-size: 12px;
  color: var(--text-secondary);
  letter-spacing: 0.5px;
}

/* ===== Bottom Grid ===== */
.dash-bottom-grid {
  display: grid;
  grid-template-columns: 1fr 300px;
  gap: 16px;
  align-items: start;
}

/* ===== Action Grid ===== */
.actions-card {
  align-self: start;
}

.action-grid {
  display: grid;
  grid-template-columns: repeat(2, 1fr);
  gap: 8px;
}

.action-tile {
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 10px;
  padding: 20px 12px;
  border-radius: var(--radius-md);
  border: 1px solid transparent;
  background: transparent;
  cursor: pointer;
  transition: all 0.2s var(--ease);
  text-align: center;
}

.action-tile:hover {
  background: rgba(255, 255, 255, 0.03);
  border-color: var(--border);
}

.action-tile span {
  font-size: 12px;
  color: var(--text-secondary);
  font-weight: 400;
}

.action-tile-icon {
  width: 40px;
  height: 40px;
  border-radius: 10px;
  display: flex;
  align-items: center;
  justify-content: center;
  font-size: 18px;
  transition: transform 0.2s var(--ease-spring);
}

.action-tile:hover .action-tile-icon {
  transform: scale(1.1);
}

.action-tile-icon.gold { background: var(--accent-muted); color: var(--accent); }
.action-tile-icon.blue { background: rgba(96, 165, 250, 0.1); color: #60a5fa; }
.action-tile-icon.green { background: rgba(74, 222, 128, 0.1); color: #4ade80; }
.action-tile-icon.amber { background: rgba(251, 191, 36, 0.1); color: #fbbf24; }

/* ===== Responsive ===== */
@media (max-width: 1200px) {
  .bento-6col { grid-template-columns: repeat(2, 1fr); }
  .dash-bottom-grid { grid-template-columns: 1fr; }
}

@media (max-width: 768px) {
  .bento-3col,
  .bento-4col,
  .bento-6col {
    grid-template-columns: 1fr;
  }
  .span-2 { grid-column: span 1; }
  .row-2 { grid-row: span 1; }
}
</style>
